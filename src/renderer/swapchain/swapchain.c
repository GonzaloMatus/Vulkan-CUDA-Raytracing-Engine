#include <stdlib.h>
#include <limits.h>
#include <stdio.h>
#include "swapchain.h"
#include "../vulkan/vk_context.h"

typedef struct {
    VkSurfaceCapabilitiesKHR capabilities;

    uint32_t format_count;
    VkSurfaceFormatKHR* formats;

    uint32_t present_mode_count;
    VkPresentModeKHR* present_modes;
} SwapchainSupport;


static bool query_swapchain_support(VkPhysicalDevice physical_device, VkSurfaceKHR surface, SwapchainSupport* support) {
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &support->capabilities);

    vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &support->format_count, NULL);

    if (support->format_count == 0) {
        return false;
    }

    support->formats = malloc(sizeof(VkSurfaceFormatKHR) * support->format_count);

    vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &support->format_count, support->formats);

    vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &support->present_mode_count, NULL);

    if (support->present_mode_count == 0) {
        free(support->formats);
        return false;
    }

    support->present_modes = malloc( sizeof(VkPresentModeKHR) * support->present_mode_count);

    vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &support->present_mode_count, support->present_modes);

    return true;
}


static VkSurfaceFormatKHR choose_surface_format(const SwapchainSupport* support) {
    for (uint32_t i = 0; i < support->format_count; i++) {
        if (support->formats[i].format == VK_FORMAT_R8G8B8A8_UNORM && support->formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return support->formats[i];
        }
    }
    return support->formats[0];
}


static VkPresentModeKHR choose_present_mode(const SwapchainSupport* support) {
    for (uint32_t i = 0; i < support->present_mode_count; i++) {
        if (support->present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
            return VK_PRESENT_MODE_MAILBOX_KHR;
        }
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

static VkExtent2D choose_extent(const VkSurfaceCapabilitiesKHR* caps, uint32_t width, uint32_t height) {
    if (caps->currentExtent.width != UINT32_MAX) {
        return caps->currentExtent;
    }

    VkExtent2D extent = { width, height };

    if (extent.width < caps->minImageExtent.width)
        extent.width = caps->minImageExtent.width;
    if (extent.width > caps->maxImageExtent.width)
        extent.width = caps->maxImageExtent.width;

    if (extent.height < caps->minImageExtent.height)
        extent.height = caps->minImageExtent.height;
    if (extent.height > caps->maxImageExtent.height)
        extent.height = caps->maxImageExtent.height;

    return extent;
}


static void free_swapchain_support(SwapchainSupport* support) {
    free(support->formats);
    free(support->present_modes);
}


bool swapchain_create(Swapchain* swapchain, const VulkanContext* ctx, uint32_t width, uint32_t height) {
    SwapchainSupport support;
    if (!query_swapchain_support(ctx->physical_device, ctx->surface, &support)) {
        fprintf(stderr, "Swapchain not supported\n");
        return false;
    }

    VkSurfaceFormatKHR format = choose_surface_format(&support);
    VkPresentModeKHR present_mode = choose_present_mode(&support);
    VkExtent2D extent = choose_extent(&support.capabilities, width, height);

    uint32_t image_count = support.capabilities.minImageCount + 1;

    if (support.capabilities.maxImageCount > 0 && image_count > support.capabilities.maxImageCount) {
        image_count = support.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR info = {0};
    info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    info.surface = ctx->surface;
    info.minImageCount = image_count;
    info.imageFormat = format.format;
    info.imageColorSpace = format.colorSpace;
    info.imageExtent = extent;
    info.imageArrayLayers = 1;
    info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    if (ctx->graphics_queue_family != ctx->present_queue_family) {
        info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        info.queueFamilyIndexCount = 2;
        info.pQueueFamilyIndices = (uint32_t[]){ctx->graphics_queue_family, ctx->present_queue_family};
    } else {
        info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }
    info.preTransform = support.capabilities.currentTransform;
    info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    info.presentMode = present_mode;
    info.clipped = VK_TRUE;

    if (vkCreateSwapchainKHR(ctx->device, &info, NULL, &swapchain->handle) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create swapchain\n");
        return false;
    }

    vkGetSwapchainImagesKHR(ctx->device, swapchain->handle, &swapchain->image_count, NULL);

    swapchain->images = malloc(sizeof(VkImage) * swapchain->image_count);

    vkGetSwapchainImagesKHR(ctx->device, swapchain->handle, &swapchain->image_count, swapchain->images);

    swapchain->image_format = format.format;
    swapchain->extent = extent;

    swapchain->image_views = malloc(sizeof(VkImageView) * swapchain->image_count);

    for (uint32_t i = 0; i < swapchain->image_count; i++) {
        VkImageViewCreateInfo view = {0};
        view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        view.image = swapchain->images[i];
        view.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view.format = swapchain->image_format;
        view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        view.subresourceRange.baseMipLevel = 0;
        view.subresourceRange.levelCount = 1;
        view.subresourceRange.baseArrayLayer = 0;
        view.subresourceRange.layerCount = 1;

        if (vkCreateImageView(ctx->device, &view, NULL, &swapchain->image_views[i]) != VK_SUCCESS) {
            fprintf(stderr, "Failed to create image view\n");
            return false;
        }
    }

    free_swapchain_support(&support);

    printf("Swapchain created (%u images)\n", swapchain->image_count);
    return true;
}


void swapchain_destroy(Swapchain* swapchain, const VulkanContext* ctx) {
    for (uint32_t i = 0; i < swapchain->image_count; i++) {
        vkDestroyImageView(ctx->device, swapchain->image_views[i], NULL);
    }

    free(swapchain->image_views);
    free(swapchain->images);

    vkDestroySwapchainKHR(ctx->device, swapchain->handle, NULL);

    swapchain->handle = VK_NULL_HANDLE;
}
