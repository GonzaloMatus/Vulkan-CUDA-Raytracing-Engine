// compute_metal.m
#import <Metal/Metal.h>
#include "compute.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static NSMutableDictionary* g_buffers = nil;  // NSValue(ptr) -> MTLBuffer*
static id<MTLDevice>        g_device  = nil;
static dispatch_queue_t     g_lock    = nil;

static id<MTLDevice> get_device(void) {
    if (!g_device) {
        g_device = MTLCreateSystemDefaultDevice();
        if (g_device)
            printf("[compute_metal] Device: %s\n", [[g_device name] UTF8String]);
    }
    return g_device;
}

static bool ensure_init(void) {
    if (!g_buffers) {
        g_buffers = [[NSMutableDictionary alloc] init];
        g_lock    = dispatch_queue_create("compute_metal.lock", DISPATCH_QUEUE_SERIAL);
    }
    return get_device() != nil;
}

bool compute_init(void) {
    return ensure_init();
}

void compute_shutdown(void) {
    if (g_buffers) { [g_buffers removeAllObjects]; g_buffers = nil; }
}

bool compute_alloc(void** ptr, size_t size) {
    if (!ensure_init()) return false;
    if (!ptr) return false;

    if (size == 0) {
        *ptr = NULL;
        return true;
    }

    id<MTLDevice> dev = get_device();
    if (!dev) return false;

    id<MTLBuffer> buf = [dev newBufferWithLength:size
                                         options:MTLResourceStorageModeShared];
    if (!buf) {
        fprintf(stderr, "[compute_metal] FAILED to allocate %zu bytes\n", size);
        return false;
    }

    void* contents = [buf contents];
    if (!contents) {
        fprintf(stderr, "[compute_metal] buffer contents is NULL for %zu bytes\n", size);
        return false;
    }
    *ptr = contents;

    dispatch_sync(g_lock, ^{
        g_buffers[[NSValue valueWithPointer:contents]] = buf;
    });
    return true;
}

bool compute_free(void* ptr) {
    if (!ptr) return true;
    dispatch_sync(g_lock, ^{
        [g_buffers removeObjectForKey:[NSValue valueWithPointer:ptr]];
    });
    return true;
}

bool compute_memcpy_to(void* dst, const void* src, size_t size) {
    if (!dst || !src || size == 0) return true;
    memcpy(dst, src, size);
    return true;
}

bool compute_memcpy_from(void* dst, const void* src, size_t size) {
    if (!dst || !src || size == 0) return true;
    memcpy(dst, src, size);
    return true;
}

bool compute_memset(void* ptr, int value, size_t size) {
    if (!ptr || size == 0) return true;
    memset(ptr, value, size);
    return true;
}

bool compute_sync(void) {
    return true;
}

// Devuelve el MTLBuffer* que respalda a un puntero device.
// El raytracer lo usa para pasarlo al kernel como [[buffer(N)]].
void* compute_get_native_buffer(void* ptr) {
    if (!ptr) return NULL;
    __block id<MTLBuffer> buf = nil;
    dispatch_sync(g_lock, ^{
        buf = g_buffers[[NSValue valueWithPointer:ptr]];
    });
    return (__bridge void*)buf;
}