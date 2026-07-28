#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "../../camera/camera.h"
#include "../../scene/scene_gpu.h"
#include "../cuda_renderer/cuda_renderer.h"

#ifdef __cplusplus
extern "C" {
#endif


void execute_raygen_kernel(CudaRenderTarget* rt, uint32_t frame_index, const Camera* camera, const SceneGPU* scene);

#ifdef __cplusplus
}
#endif
