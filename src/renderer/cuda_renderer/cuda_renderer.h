#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "../../camera/camera.h"
#include "../../scene/scene_gpu.h"
#include "../../cuda_api/cuda_api.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t width;
    uint32_t height;

    void* device_ptr;              // puntero CUDA al buffer Vulkan (output final)
    size_t size;

    // --- NUEVO: accumulation buffer ---
    void* accumulation_buffer;   // puntero a buffer CUDA para acumulación
    size_t accumulation_size;      // tamaño en bytes

    // CUDA external memory
    CudaExternalMemory cuda_external_memory;
} CudaRenderTarget;


bool cuda_renderer_init(CudaRenderTarget* rt, void* win32_handle, size_t size, uint32_t width, uint32_t height);
void cuda_renderer_render(CudaRenderTarget* rt, uint32_t frame_index, const Camera* camera, const SceneGPU* scene);
void cuda_renderer_shutdown(CudaRenderTarget* rt);
void cudaDeviceSynchronize_wrapper(void);  // Sincroniza GPU desde C

#ifdef __cplusplus
}
#endif
