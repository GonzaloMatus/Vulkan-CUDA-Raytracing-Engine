#pragma once
#include <vulkan/vulkan.h>
#include <stdbool.h>
#include "../../core/window.h"

typedef struct {
    VkInstance instance;
    VkPhysicalDevice physical_device;
    VkDevice device;
    VkSurfaceKHR surface;
    VkQueue graphics_queue;
    VkQueue present_queue;
    uint32_t graphics_queue_family;
    uint32_t present_queue_family;
#ifndef NDEBUG
    VkDebugUtilsMessengerEXT debug_messenger;
    bool enable_validation;
#endif

} VulkanContext;


bool vk_context_init(VulkanContext* ctx, const Window* window);
void vk_context_shutdown(VulkanContext* ctx);
