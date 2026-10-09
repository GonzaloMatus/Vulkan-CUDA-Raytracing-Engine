#pragma once
#include <stdint.h>
#include "../../camera/camera.h"
#include "../../scene/scene_gpu.h"

#ifdef __cplusplus
extern "C" {
#endif

void execute_raygen_kernel(void* device_ptr, void* accumulation_buffer,
                           uint32_t width, uint32_t height,
                           uint32_t frame_index,
                           const Camera* cam, const SceneGPU* scene_gpu);

#ifdef __cplusplus
}
#endif