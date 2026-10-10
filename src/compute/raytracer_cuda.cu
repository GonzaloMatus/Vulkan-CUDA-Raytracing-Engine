#include "raytracer.h"
#include "../cuda_api/cuda_api.h"
#include "../renderer/raygen_kernel/raygen_kernel.h"
#include <stdlib.h>
#include <stdio.h>

typedef struct {
    void* device_ptr;
    void* accumulation_buffer;
    size_t accumulation_size;
    CudaExternalMemory external_memory;
} CudaBackend;

bool raytracer_create(Raytracer* rt, void* native_buffer, size_t size,
                      uint32_t width, uint32_t height) {
    rt->width         = width;
    rt->height        = height;
    rt->native_buffer = native_buffer;

    CudaBackend* be = (CudaBackend*)calloc(1, sizeof(CudaBackend));
    if (!be) return false;

    if (!cuda_import_external_memory_win32(&be->external_memory, native_buffer, size)) {
        free(be);
        return false;
    }
    if (!cuda_external_memory_get_buffer(&be->device_ptr, be->external_memory, size)) {
        cuda_external_memory_destroy(&be->external_memory);
        free(be);
        return false;
    }
    printf("CUDA imported Vulkan buffer (%zu bytes)\n", size);

    be->accumulation_size = width * height * 4 * sizeof(float);
    if (!cuda_alloc(&be->accumulation_buffer, be->accumulation_size)) {
        printf("Failed to allocate accumulation buffer\n");
        cuda_external_memory_destroy(&be->external_memory);
        free(be);
        return false;
    }
    printf("CUDA accumulation buffer allocated (%zu bytes)\n", be->accumulation_size);

    rt->backend_state = be;
    return true;
}

void raytracer_render(Raytracer* rt, uint32_t frame_index,
                      const Camera* camera, const SceneGPU* scene) {
    CudaBackend* be = (CudaBackend*)rt->backend_state;
    execute_raygen_kernel(be->device_ptr, be->accumulation_buffer,
                          rt->width, rt->height,
                          frame_index, camera, scene);
}

void raytracer_sync(Raytracer* rt) {
    (void)rt;
    cuda_sync();
}

void raytracer_destroy(Raytracer* rt) {
    CudaBackend* be = (CudaBackend*)rt->backend_state;
    if (!be) return;

    if (be->external_memory.handle) cuda_external_memory_destroy(&be->external_memory);
    if (be->accumulation_buffer)    cuda_free(be->accumulation_buffer);

    free(be);
    rt->backend_state = NULL;
}