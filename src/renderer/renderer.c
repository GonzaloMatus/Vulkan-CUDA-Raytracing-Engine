#include <stdio.h>
#include "renderer.h"
#include "vulkan/vk_context.h"
#include <stdlib.h>
#include "swapchain/swapchain.h"
#include "command_buffers/command_buffers.h"
#include "render_target/render_target.h"
#include <string.h>
#include "cuda_renderer/cuda_renderer.h"
#include "../core/time.h"
#include "sync/sync.h"
#include "gpu_render_buffer/gpu_render_buffer.h"
#include "tonemap_pipeline/tonemap_pipeline.h"
#include "../scene/scene.h"
#include "../camera/camera.h"
#include "../scene/scene_gpu.h"

void renderer_init(Renderer* renderer, Window* window) {
    renderer->window = window;
    renderer->frame_index = 0;

    if (!vk_context_init(&renderer->vk, window)) {
        fprintf(stderr, "Failed to initialize Vulkan\n");
        exit(EXIT_FAILURE);
    }

    if (!swapchain_create(&renderer->swapchain, &renderer->vk, window->width, window->height)) {
        fprintf(stderr, "Failed to create swapchain\n");
        exit(EXIT_FAILURE);
    }
    
    if (!command_buffers_create(&renderer->command_buffers, &renderer->vk, &renderer->swapchain)) {
        fprintf(stderr, "Failed to create command buffers\n");
        exit(EXIT_FAILURE);
    }

    if (!render_target_hdr_create(&renderer->hdr_render_target, &renderer->vk, window->width, window->height)) {
        fprintf(stderr, "Failed to create render target\n");
        exit(EXIT_FAILURE);
    }
    
    if (!render_target_create(&renderer->render_target, &renderer->vk, window->width, window->height)) {
        fprintf(stderr, "Failed to create render target\n");
        exit(EXIT_FAILURE);
    }

    if (!tonemap_pipeline_create(&renderer->tonemap_pipeline, &renderer->vk, renderer->hdr_render_target.view, renderer->render_target.view)) {
        fprintf(stderr, "Failed to create render target\n");
        exit(EXIT_FAILURE);
    }
    
    if (!gpu_render_buffer_create(&renderer->gpu_render_buffer, renderer->vk.physical_device, renderer->vk.device, window->width, window->height)) {
        fprintf(stderr, "Failed to create GPU render buffer\n");
        exit(EXIT_FAILURE);
    }

    if (!cuda_renderer_init(&renderer->cuda_render_target, renderer->gpu_render_buffer.win32_handle, renderer->gpu_render_buffer.size, window->width, window->height)) {
        fprintf(stderr, "Failed to initialize cuda_render_target.\n");
        exit(EXIT_FAILURE);
    }

    if (!command_buffers_record(&renderer->command_buffers, &renderer->vk, &renderer->swapchain, renderer->gpu_render_buffer.buffer, &renderer->hdr_render_target, &renderer->render_target, &renderer->tonemap_pipeline)) {
        fprintf(stderr, "Failed to record command buffers\n");
        exit(EXIT_FAILURE);
    }

    if (!sync_create(&renderer->sync_objects, &renderer->vk)) {
        fprintf(stderr, "Failed to create sync objects\n");
        exit(EXIT_FAILURE);
    }

    printf("Renderer initialized\n");
}

void renderer_recreate_swapchain(Renderer* renderer) {
    VulkanContext* ctx = &renderer->vk;

    vkDeviceWaitIdle(ctx->device);
    cuda_renderer_shutdown(&renderer->cuda_render_target);
    gpu_render_buffer_destroy(&renderer->gpu_render_buffer, ctx->device);
    tonemap_pipeline_destroy(&renderer->tonemap_pipeline, ctx);
    render_target_hdr_destroy(&renderer->hdr_render_target, &renderer->vk);
    render_target_destroy(&renderer->render_target, ctx);
    command_buffers_destroy(&renderer->command_buffers, ctx);
    swapchain_destroy(&renderer->swapchain, ctx);
    
    if (!swapchain_create(&renderer->swapchain, &renderer->vk, renderer->window->width, renderer->window->height)) {
        fprintf(stderr, "Failed to create swapchain\n");
        exit(EXIT_FAILURE);
    }

    if (!command_buffers_create(&renderer->command_buffers, &renderer->vk, &renderer->swapchain)) {
        fprintf(stderr, "Failed to create command buffers\n");
        exit(EXIT_FAILURE);
    }

    if (!render_target_hdr_create(&renderer->hdr_render_target, &renderer->vk, renderer->window->width, renderer->window->height)) {
        fprintf(stderr, "Failed to create render target\n");
        exit(EXIT_FAILURE);
    }

    if (!render_target_create(&renderer->render_target, &renderer->vk, renderer->window->width, renderer->window->height)) {
        fprintf(stderr, "Failed to create render target\n");
        exit(EXIT_FAILURE);
    }

    if (!tonemap_pipeline_create(&renderer->tonemap_pipeline, &renderer->vk, renderer->hdr_render_target.view, renderer->render_target.view)) {
        fprintf(stderr, "Failed to create render target\n");
        exit(EXIT_FAILURE);
    }

    if (!gpu_render_buffer_create(&renderer->gpu_render_buffer, renderer->vk.physical_device, renderer->vk.device, renderer->window->width, renderer->window->height)) {
        fprintf(stderr, "Failed to recreate GPU render buffer\n");
        exit(EXIT_FAILURE);
    }
    
    if (!cuda_renderer_init(&renderer->cuda_render_target, renderer->gpu_render_buffer.win32_handle, renderer->gpu_render_buffer.size, renderer->window->width, renderer->window->height)) {
        fprintf(stderr, "Failed to initialize cuda_render_target.\n");
        exit(EXIT_FAILURE);
    }

    if (!command_buffers_record(&renderer->command_buffers, &renderer->vk, &renderer->swapchain, renderer->gpu_render_buffer.buffer, &renderer->hdr_render_target, &renderer->render_target, &renderer->tonemap_pipeline)) {

        fprintf(stderr, "Failed to record command buffers\n");
        exit(EXIT_FAILURE);
    }
}

void renderer_shutdown(Renderer* renderer) {
    cuda_renderer_shutdown(&renderer->cuda_render_target);
    vkDeviceWaitIdle(renderer->vk.device);
    gpu_render_buffer_destroy(&renderer->gpu_render_buffer, renderer->vk.device);
    sync_destroy(&renderer->sync_objects, &renderer->vk);
    tonemap_pipeline_destroy(&renderer->tonemap_pipeline, &renderer->vk);
    render_target_hdr_destroy(&renderer->hdr_render_target, &renderer->vk);
    render_target_destroy(&renderer->render_target, &renderer->vk);
    command_buffers_destroy(&renderer->command_buffers, &renderer->vk);
    swapchain_destroy(&renderer->swapchain, &renderer->vk);
    vk_context_shutdown(&renderer->vk);
    printf("Renderer shutdown\n");
}


bool renderer_begin_frame(Renderer* renderer, uint32_t* image_index, const Camera* camera, const SceneGPU* scene) {
    const VulkanContext* ctx = &renderer->vk;
    const Swapchain* swapchain = &renderer->swapchain;
    const SyncObjects* sync = &renderer->sync_objects;
    
    if (renderer->window->width == 0 || renderer->window->height == 0){
        return false;
    }

    cuda_renderer_render(&renderer->cuda_render_target, renderer->frame_index, camera, scene); //async
    
    vkWaitForFences(ctx->device, 1, &sync->in_flight, VK_TRUE, UINT64_MAX);
    vkResetFences(ctx->device, 1, &sync->in_flight);

    VkResult res = vkAcquireNextImageKHR(ctx->device, swapchain->handle, UINT64_MAX, sync->image_available, VK_NULL_HANDLE, image_index);

    if (res == VK_ERROR_OUT_OF_DATE_KHR) {
        renderer_recreate_swapchain(renderer);
        return false;
    }

    if (res != VK_SUCCESS && res != VK_SUBOPTIMAL_KHR) {
        fprintf(stderr, "Failed to acquire image\n");
        return false;
    }

    return true;
}


void renderer_draw_frame(Renderer* renderer, uint32_t image_index) {
    cudaDeviceSynchronize_wrapper();
    const VulkanContext* ctx = &renderer->vk;
    const CommandBuffers* cmd = &renderer->command_buffers;
    const SyncObjects* sync = &renderer->sync_objects;

    VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;

    VkSubmitInfo submit = {0};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.waitSemaphoreCount = 1;
    submit.pWaitSemaphores = &sync->image_available;
    submit.pWaitDstStageMask = &wait_stage;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &cmd->buffers[image_index];
    submit.signalSemaphoreCount = 1;
    submit.pSignalSemaphores = &sync->render_finished;

    vkQueueSubmit(ctx->graphics_queue, 1, &submit, sync->in_flight);
}


void renderer_end_frame(Renderer* renderer, uint32_t image_index) {
    const VulkanContext* ctx = &renderer->vk;
    const Swapchain* swapchain = &renderer->swapchain;
    const SyncObjects* sync = &renderer->sync_objects;

    VkPresentInfoKHR present = {0};
    present.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present.waitSemaphoreCount = 1;
    present.pWaitSemaphores = &sync->render_finished;
    present.swapchainCount = 1;
    present.pSwapchains = &swapchain->handle;
    present.pImageIndices = &image_index;

    VkResult res = vkQueuePresentKHR(ctx->present_queue, &present);

    if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR) {
        renderer_recreate_swapchain(renderer);
    }

    renderer->frame_index++;
}
