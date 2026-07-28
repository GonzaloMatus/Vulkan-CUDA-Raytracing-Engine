#pragma once
#include <vulkan/vulkan.h>
#include <stdbool.h>

#include "../vulkan/vk_context.h"

typedef struct {
    VkSemaphore image_available;
    VkSemaphore render_finished;
    VkFence in_flight;
} SyncObjects;


bool sync_create(SyncObjects* sync, const VulkanContext* ctx);
void sync_destroy(SyncObjects* sync, const VulkanContext* ctx);
