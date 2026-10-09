#pragma once
#include <vulkan/vulkan.h>
#include <stdbool.h>
#include <stdint.h>

#include "../vulkan/vk_context.h"

#define MAX_SWAPCHAIN_IMAGES 8

typedef struct {
    // Un semáforo "image available" es suficiente mientras esperamos al fence
    // antes de adquirir la siguiente imagen (single frame in flight).
    VkSemaphore image_available;

    // Un semáforo "render finished" por imagen del swapchain. Evita reutilizar
    // el semáforo mientras el present engine aún lo está usando.
    VkSemaphore render_finished[MAX_SWAPCHAIN_IMAGES];
    uint32_t    swapchain_image_count;

    VkFence     in_flight;
} SyncObjects;


bool sync_create(SyncObjects* sync, const VulkanContext* ctx, uint32_t swapchain_image_count);
void sync_destroy(SyncObjects* sync, const VulkanContext* ctx);