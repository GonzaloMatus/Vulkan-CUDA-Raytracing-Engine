#include "render_target_hdr.h"
#include <string.h>
#include <stdio.h>

static uint32_t find_memory_type(VkPhysicalDevice physical_device, uint32_t type_filter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties mem_props;
    vkGetPhysicalDeviceMemoryProperties(physical_device, &mem_props);

    for (uint32_t i = 0; i < mem_props.memoryTypeCount; i++) {
        if ((type_filter & (1 << i)) &&
            (mem_props.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    return UINT32_MAX;
}

bool render_target_hdr_create(RenderTargetHDR* target, const VulkanContext* ctx, uint32_t width, uint32_t height) {
    memset(target, 0, sizeof(*target));

    target->width  = width;
    target->height = height;
    target->format = VK_FORMAT_R16G16B16A16_SFLOAT;

    /* --------------------------------------------------
       Crear imagen HDR
       -------------------------------------------------- */
    VkImageCreateInfo image_info = {0};
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.extent.width  = width;
    image_info.extent.height = height;
    image_info.extent.depth  = 1;
    image_info.mipLevels     = 1;
    image_info.arrayLayers   = 1;
    image_info.format        = target->format;
    image_info.tiling        = VK_IMAGE_TILING_OPTIMAL;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_info.usage =
        VK_IMAGE_USAGE_TRANSFER_DST_BIT |  // buffer → imagen
        VK_IMAGE_USAGE_STORAGE_BIT |       // compute shader (tonemap)
        VK_IMAGE_USAGE_SAMPLED_BIT;         // futuro (postprocesado)
    image_info.samples     = VK_SAMPLE_COUNT_1_BIT;
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(ctx->device, &image_info, NULL, &target->image) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create HDR render target image\n");
        return false;
    }

    /* --------------------------------------------------
       Memoria
       -------------------------------------------------- */
    VkMemoryRequirements mem_req;
    vkGetImageMemoryRequirements(ctx->device, target->image, &mem_req);

    uint32_t memory_type = find_memory_type(
        ctx->physical_device,
        mem_req.memoryTypeBits,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    );

    if (memory_type == UINT32_MAX) {
        fprintf(stderr, "Failed to find suitable memory type for HDR render target\n");
        return false;
    }

    VkMemoryAllocateInfo alloc = {0};
    alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc.allocationSize  = mem_req.size;
    alloc.memoryTypeIndex = memory_type;

    if (vkAllocateMemory(ctx->device, &alloc, NULL, &target->memory) != VK_SUCCESS) {
        fprintf(stderr, "Failed to allocate HDR render target memory\n");
        return false;
    }

    vkBindImageMemory(ctx->device, target->image, target->memory, 0);

    /* --------------------------------------------------
       Image view
       -------------------------------------------------- */
    VkImageViewCreateInfo view = {0};
    view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view.image = target->image;
    view.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view.format = target->format;
    view.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    view.subresourceRange.baseMipLevel   = 0;
    view.subresourceRange.levelCount     = 1;
    view.subresourceRange.baseArrayLayer = 0;
    view.subresourceRange.layerCount     = 1;

    if (vkCreateImageView(ctx->device, &view, NULL, &target->view) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create HDR render target image view\n");
        return false;
    }

    printf("HDR RenderTarget created (%ux%u, RGBA16F)\n", width, height);
    return true;
}

void render_target_hdr_destroy(RenderTargetHDR* target, const VulkanContext* ctx) {
    if (target->view) {
        vkDestroyImageView(ctx->device, target->view, NULL);
    }

    if (target->image) {
        vkDestroyImage(ctx->device, target->image, NULL);
    }

    if (target->memory) {
        vkFreeMemory(ctx->device, target->memory, NULL);
    }

    memset(target, 0, sizeof(*target));
}