#pragma once
#include <stdint.h>
#include "core/window.h"
#include "vulkan/vk_context.h"
#include "swapchain/swapchain.h"
#include "command_buffers/command_buffers.h"
#include "sync/sync.h"
#include "render_target/render_target.h"
#include "../compute/raytracer.h"
#include "gpu_render_buffer/gpu_render_buffer.h"
#include "render_target_hdr/render_target_hdr.h"
#include "tonemap_pipeline/tonemap_pipeline.h"
#include "../scene/scene.h"
#include "../camera/camera.h"
#include "../scene/scene_gpu.h"

typedef struct {
    const Window* window;
    VulkanContext vk;
    GpuRenderBuffer gpu_render_buffer;
    Raytracer raytracer;
    RenderTargetHDR hdr_render_target;
    TonemapPipeline tonemap_pipeline;
    RenderTarget render_target;
    Swapchain swapchain;
    CommandBuffers command_buffers;
    SyncObjects sync_objects;
    uint32_t frame_index;
} Renderer;


void renderer_init(Renderer* renderer, Window* window);
bool renderer_begin_frame(Renderer* renderer, uint32_t* image_index, const Camera* camera, const SceneGPU* scene);
void renderer_recreate_swapchain(Renderer* renderer);
void renderer_draw_frame(Renderer* renderer, uint32_t image_index);
void renderer_end_frame(Renderer* renderer, uint32_t image_index);
void renderer_shutdown(Renderer* renderer);