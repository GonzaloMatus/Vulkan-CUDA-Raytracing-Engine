#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "../camera/camera.h"
#include "../scene/scene_gpu.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t width;
    uint32_t height;
    void*    native_buffer;    // handle nativo del buffer Vulkan (HANDLE en Win32)
    void*    backend_state;    // estado interno opaco del backend
} Raytracer;

bool raytracer_create(Raytracer* rt, void* native_buffer, size_t size,
                      uint32_t width, uint32_t height);
void raytracer_render(Raytracer* rt, uint32_t frame_index,
                      const Camera* camera, const SceneGPU* scene);
void raytracer_sync(Raytracer* rt);
void raytracer_destroy(Raytracer* rt);

#ifdef __cplusplus
}
#endif