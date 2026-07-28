#pragma once
#include <vulkan/vulkan.h>
#include "../vulkan/vk_context.h"
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    VkImage        image;
    VkDeviceMemory memory;
    VkImageView    view;
    VkFormat       format;
    uint32_t       width;
    uint32_t       height;
} RenderTargetHDR;

bool render_target_hdr_create(
    RenderTargetHDR* target,
    const VulkanContext* ctx,
    uint32_t width,
    uint32_t height
);

void render_target_hdr_destroy(
    RenderTargetHDR* target,
    const VulkanContext* ctx
);
