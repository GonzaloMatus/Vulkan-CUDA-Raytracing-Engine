#pragma once
#include <vulkan/vulkan.h>
#include <stdint.h>
#include <stdbool.h>
#include "../vulkan/vk_context.h"

typedef struct {
    VkSwapchainKHR handle;
    VkExtent2D extent;
    VkImage* images;
    VkImageView* image_views;
    VkFormat image_format;
    uint32_t image_count;
} Swapchain;


bool swapchain_create(Swapchain* swapchain, const VulkanContext* ctx, uint32_t width, uint32_t height);

void swapchain_destroy(Swapchain* swapchain, const VulkanContext* ctx);