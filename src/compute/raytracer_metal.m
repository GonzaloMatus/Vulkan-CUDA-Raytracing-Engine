// raytracer_metal.m
#import <Metal/Metal.h>
#include "raytracer.h"
#include "compute.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>

#ifndef ENGINE_SHADER_DIR
#  define ENGINE_SHADER_DIR "shaders"
#endif

typedef struct {
    id<MTLDevice>               device;
    id<MTLCommandQueue>         queue;
    id<MTLComputePipelineState> pipeline;
    id<MTLBuffer>               output;
    id<MTLBuffer>               accumulation;

    uint32_t width;
    uint32_t height;
    size_t   size;
    void*    mapped;
} MetalBackend;

// Espejo del struct SceneCounts en MSL. Debe coincidir byte a byte.
typedef struct {
    uint64_t materials_size;
    uint64_t objects_size;
    uint64_t bvh_object_indexes_size;
    uint64_t bvh_nodes_size;
    uint64_t spheres_size;
    uint64_t triangles_size;
    uint64_t boxes_size;
    uint64_t cylinders_size;
} SceneCounts;

bool raytracer_create(Raytracer* rt, void* native_buffer, size_t size,
                      uint32_t width, uint32_t height) {
    rt->width         = width;
    rt->height        = height;
    rt->native_buffer = native_buffer;

    MetalBackend* be = calloc(1, sizeof(MetalBackend));
    be->width  = width;
    be->height = height;
    be->size   = size;
    be->mapped = native_buffer;

    be->device = MTLCreateSystemDefaultDevice();
    if (!be->device) {
        fprintf(stderr, "[raytracer_metal] No Metal device found\n");
        free(be);
        return false;
    }
    be->queue = [be->device newCommandQueue];

    NSString* lib_path = [NSString stringWithUTF8String:
        ENGINE_SHADER_DIR "/raygen.metallib"];
    NSURL* lib_url = [NSURL fileURLWithPath:lib_path];

    NSError* err = nil;
    id<MTLLibrary> library = [be->device newLibraryWithURL:lib_url error:&err];
    if (!library) {
        fprintf(stderr, "[raytracer_metal] Failed to load %s: %s\n",
                [lib_path UTF8String], [[err localizedDescription] UTF8String]);
        free(be);
        return false;
    }

    id<MTLFunction> fn = [library newFunctionWithName:@"raygen"];
    if (!fn) {
        fprintf(stderr, "[raytracer_metal] Function raygen not found\n");
        free(be);
        return false;
    }

    be->pipeline = [be->device newComputePipelineStateWithFunction:fn error:&err];
    if (!be->pipeline) {
        fprintf(stderr, "[raytracer_metal] Failed to create pipeline: %s\n",
                [[err localizedDescription] UTF8String]);
        free(be);
        return false;
    }

    NSUInteger page_size = (NSUInteger)getpagesize();
    if (((uintptr_t)be->mapped % page_size) != 0) {
        fprintf(stderr,
                "[raytracer_metal] native_buffer %p not aligned to %lu bytes\n",
                be->mapped, (unsigned long)page_size);
        free(be);
        return false;
    }

    be->output = [be->device newBufferWithBytesNoCopy:be->mapped
                                               length:size
                                              options:MTLResourceStorageModeShared
                                          deallocator:nil];
    if (!be->output) {
        fprintf(stderr, "[raytracer_metal] Failed to wrap native buffer\n");
        free(be);
        return false;
    }

    // Accumulation buffer: float4 por píxel. StorageModePrivate porque solo
    // lo lee/escribe el kernel, nunca el CPU.
    const size_t accum_size = (size_t)width * (size_t)height * sizeof(float) * 4;
    be->accumulation = [be->device newBufferWithLength:accum_size
                                                options:MTLResourceStorageModePrivate];
    if (!be->accumulation) {
        fprintf(stderr, "[raytracer_metal] Failed to create accumulation buffer\n");
        free(be);
        return false;
    }

    printf("[raytracer_metal] init %ux%u (%zu bytes), accumulation=%zu bytes\n",
           width, height, size, accum_size);
    rt->backend_state = be;
    return true;
}

void raytracer_render(Raytracer* rt, uint32_t frame_index,
                      const Camera* camera, const SceneGPU* scene) {
    MetalBackend* be = rt->backend_state;
    if (!be) return;

    id<MTLBuffer> buf_materials   = (__bridge id<MTLBuffer>)compute_get_native_buffer(scene->materials.data);
    id<MTLBuffer> buf_objects     = (__bridge id<MTLBuffer>)compute_get_native_buffer(scene->objects.data);
    id<MTLBuffer> buf_bvh_obj_idx = (__bridge id<MTLBuffer>)compute_get_native_buffer(scene->bvh.object_indexes.data);
    id<MTLBuffer> buf_bvh_nodes   = (__bridge id<MTLBuffer>)compute_get_native_buffer(scene->bvh.nodes.data);
    id<MTLBuffer> buf_spheres     = (__bridge id<MTLBuffer>)compute_get_native_buffer(scene->spheres.data);
    id<MTLBuffer> buf_triangles   = (__bridge id<MTLBuffer>)compute_get_native_buffer(scene->triangles.data);
    id<MTLBuffer> buf_boxes       = (__bridge id<MTLBuffer>)compute_get_native_buffer(scene->boxes.data);
    id<MTLBuffer> buf_cylinders   = (__bridge id<MTLBuffer>)compute_get_native_buffer(scene->cylinders.data);

    SceneCounts counts = {
        .materials_size           = scene->materials.size,
        .objects_size             = scene->objects.size,
        .bvh_object_indexes_size  = scene->bvh.object_indexes.size,
        .bvh_nodes_size           = scene->bvh.nodes.size,
        .spheres_size             = scene->spheres.size,
        .triangles_size           = scene->triangles.size,
        .boxes_size               = scene->boxes.size,
        .cylinders_size           = scene->cylinders.size,
    };

    id<MTLCommandBuffer> cb = [be->queue commandBuffer];
    id<MTLComputeCommandEncoder> enc = [cb computeCommandEncoder];

    [enc setComputePipelineState:be->pipeline];
    [enc setBuffer:be->output        offset:0 atIndex:0];
    [enc setBytes:camera length:sizeof(Camera) atIndex:1];
    [enc setBytes:&frame_index length:sizeof(frame_index) atIndex:2];
    [enc setBuffer:buf_materials     offset:0 atIndex:3];
    [enc setBuffer:buf_objects       offset:0 atIndex:4];
    [enc setBuffer:buf_bvh_obj_idx   offset:0 atIndex:5];
    [enc setBuffer:buf_bvh_nodes     offset:0 atIndex:6];
    [enc setBuffer:buf_spheres       offset:0 atIndex:7];
    [enc setBuffer:buf_triangles     offset:0 atIndex:8];
    [enc setBuffer:buf_boxes         offset:0 atIndex:9];
    [enc setBuffer:buf_cylinders     offset:0 atIndex:10];
    [enc setBytes:&counts length:sizeof(counts) atIndex:11];
    [enc setBuffer:be->accumulation  offset:0 atIndex:12];

    const NSUInteger tw = 16;
    const NSUInteger th = 16;
    MTLSize threads_per_group = MTLSizeMake(tw, th, 1);
    MTLSize groups = MTLSizeMake(
        (be->width  + tw - 1) / tw,
        (be->height + th - 1) / th,
        1);

    [enc dispatchThreadgroups:groups threadsPerThreadgroup:threads_per_group];
    [enc endEncoding];

    [cb commit];
}

void raytracer_sync(Raytracer* rt) {
    MetalBackend* be = rt->backend_state;
    if (!be) return;
    id<MTLCommandBuffer> cb = [be->queue commandBuffer];
    [cb commit];
    [cb waitUntilCompleted];
}

void raytracer_destroy(Raytracer* rt) {
    MetalBackend* be = rt->backend_state;
    if (!be) return;

    id<MTLCommandBuffer> cb = [be->queue commandBuffer];
    [cb commit];
    [cb waitUntilCompleted];

    be->accumulation = nil;
    be->output       = nil;
    be->pipeline     = nil;
    be->queue        = nil;
    be->device       = nil;
    free(be);
    rt->backend_state = NULL;
}