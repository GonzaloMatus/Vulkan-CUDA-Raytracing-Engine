#include "sync.h"
#include <stdio.h>

bool sync_create(SyncObjects* sync, const VulkanContext* ctx) {
    VkSemaphoreCreateInfo sem = {0};
    sem.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fence = {0};
    fence.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    if (vkCreateSemaphore(ctx->device, &sem, NULL, &sync->image_available) != VK_SUCCESS ||
        vkCreateSemaphore(ctx->device, &sem, NULL, &sync->render_finished) != VK_SUCCESS ||
        vkCreateFence(ctx->device, &fence, NULL, &sync->in_flight) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create sync objects\n");
        return false;
    }

    printf("Sync objects created\n");
    return true;
}

void sync_destroy(SyncObjects* sync, const VulkanContext* ctx) {
    vkDestroyFence(ctx->device, sync->in_flight, NULL);
    vkDestroySemaphore(ctx->device, sync->render_finished, NULL);
    vkDestroySemaphore(ctx->device, sync->image_available, NULL);
}


