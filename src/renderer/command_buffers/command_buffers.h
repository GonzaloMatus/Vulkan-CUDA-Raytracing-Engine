#pragma once
#include <vulkan/vulkan.h>
#include <stdbool.h>
#include <stdint.h>
#include "../vulkan/vk_context.h"
#include "../swapchain/swapchain.h"
#include "../render_target/render_target.h"
#include "../render_target_hdr/render_target_hdr.h"
#include "../tonemap_pipeline/tonemap_pipeline.h"

typedef struct {
    VkCommandPool pool;
    VkCommandBuffer* buffers;
    uint32_t count;
} CommandBuffers;

bool command_buffers_create(CommandBuffers* cmd, const VulkanContext* ctx, const Swapchain* swapchain);

bool command_buffers_record(
    CommandBuffers* cmd,
    const VulkanContext* ctx,
    const Swapchain* swapchain,
    VkBuffer cuda_buffer,
    const RenderTargetHDR* hdr,
    const RenderTarget* ldr,
    const TonemapPipeline* tm
);


void command_buffers_destroy(CommandBuffers* cmd, const VulkanContext* ctx);
