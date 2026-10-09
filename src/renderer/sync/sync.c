#include "sync.h"
#include <stdio.h>
#include <string.h>

bool sync_create(SyncObjects* sync, const VulkanContext* ctx, uint32_t swapchain_image_count) {
    memset(sync, 0, sizeof(*sync));

    if (swapchain_image_count == 0 || swapchain_image_count > MAX_SWAPCHAIN_IMAGES) {
        fprintf(stderr, "sync_create: invalid swapchain image count %u (max %d)\n",
                swapchain_image_count, MAX_SWAPCHAIN_IMAGES);
        return false;
    }

    VkSemaphoreCreateInfo sem = {0};
    sem.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fence = {0};
    fence.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    if (vkCreateSemaphore(ctx->device, &sem, NULL, &sync->image_available) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create image_available semaphore\n");
        return false;
    }

    for (uint32_t i = 0; i < swapchain_image_count; i++) {
        if (vkCreateSemaphore(ctx->device, &sem, NULL, &sync->render_finished[i]) != VK_SUCCESS) {
            fprintf(stderr, "Failed to create render_finished semaphore %u\n", i);
            for (uint32_t j = 0; j < i; j++) {
                vkDestroySemaphore(ctx->device, sync->render_finished[j], NULL);
                sync->render_finished[j] = VK_NULL_HANDLE;
            }
            vkDestroySemaphore(ctx->device, sync->image_available, NULL);
            sync->image_available = VK_NULL_HANDLE;
            return false;
        }
    }
    sync->swapchain_image_count = swapchain_image_count;

    if (vkCreateFence(ctx->device, &fence, NULL, &sync->in_flight) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create in_flight fence\n");
        for (uint32_t i = 0; i < swapchain_image_count; i++) {
            vkDestroySemaphore(ctx->device, sync->render_finished[i], NULL);
        }
        vkDestroySemaphore(ctx->device, sync->image_available, NULL);
        memset(sync, 0, sizeof(*sync));
        return false;
    }

    printf("Sync objects created (%u render_finished semaphores)\n", swapchain_image_count);
    return true;
}

void sync_destroy(SyncObjects* sync, const VulkanContext* ctx) {
    if (sync->in_flight) {
        vkDestroyFence(ctx->device, sync->in_flight, NULL);
        sync->in_flight = VK_NULL_HANDLE;
    }

    for (uint32_t i = 0; i < sync->swapchain_image_count; i++) {
        if (sync->render_finished[i]) {
            vkDestroySemaphore(ctx->device, sync->render_finished[i], NULL);
            sync->render_finished[i] = VK_NULL_HANDLE;
        }
    }
    sync->swapchain_image_count = 0;

    if (sync->image_available) {
        vkDestroySemaphore(ctx->device, sync->image_available, NULL);
        sync->image_available = VK_NULL_HANDLE;
    }
}