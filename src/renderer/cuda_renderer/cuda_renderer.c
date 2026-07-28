#include "cuda_renderer.h"
#include <stdio.h>
#include <math.h>
#include "../../cuda_api/cuda_api.h"
#include "../../camera/camera.h"
#include "../../scene/scene_gpu.h"
#include "../raygen_kernel/raygen_kernel.h"

bool cuda_renderer_init(CudaRenderTarget* rt, void* win32_handle, size_t size, uint32_t width, uint32_t height) {
    rt->width  = width;
    rt->height = height;
    rt->size   = size;

    // Importar memoria externa de Vulkan (buffer de salida)
    if (!cuda_import_external_memory_win32(&rt->cuda_external_memory, win32_handle, size)) {
        return false;
    }

    if (!cuda_external_memory_get_buffer(&rt->device_ptr, rt->cuda_external_memory, size)) {
        cuda_external_memory_destroy(&rt->cuda_external_memory);
        return false;
    }

    printf("CUDA imported Vulkan buffer (%zu bytes)\n", size);

    // --- NUEVO: Crear accumulation buffer ---
    rt->accumulation_size = width * height * 4 * sizeof(float);
    if (!cuda_alloc(&rt->accumulation_buffer, rt->accumulation_size)) {
        printf("Failed to allocate accumulation buffer\n");
        cuda_external_memory_destroy(&rt->cuda_external_memory);
        rt->device_ptr = NULL;
        return false;
    }

    printf("CUDA accumulation buffer allocated (%zu bytes)\n", rt->accumulation_size);
    return true;
}



void cuda_renderer_shutdown(CudaRenderTarget* rt) {
    if (rt->cuda_external_memory.handle) {
        cuda_external_memory_destroy(&rt->cuda_external_memory);
    }
    rt->device_ptr = NULL;
    rt->cuda_external_memory.handle = NULL;

    if (rt->accumulation_buffer) {
        cuda_free(rt->accumulation_buffer);
        rt->accumulation_buffer = NULL;
        rt->accumulation_size = 0;
    }
}


void cuda_renderer_render(CudaRenderTarget* rt, uint32_t frame_index, const Camera* camera, const SceneGPU* scene) {
    execute_raygen_kernel(rt, frame_index, camera, scene);
}