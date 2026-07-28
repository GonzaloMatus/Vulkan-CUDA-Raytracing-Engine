#pragma once
#include <vulkan/vulkan.h>
#include <stdint.h>
#include <stdbool.h>

#include "../vulkan/vk_context.h"

typedef struct {
    VkImage image;
    VkDeviceMemory memory;
    VkImageView view;
    VkFormat format;
    uint32_t width;
    uint32_t height;
} RenderTarget;


bool render_target_create(RenderTarget* target, const VulkanContext* ctx, uint32_t width, uint32_t height);

void render_target_destroy(RenderTarget* target, const VulkanContext* ctx);
