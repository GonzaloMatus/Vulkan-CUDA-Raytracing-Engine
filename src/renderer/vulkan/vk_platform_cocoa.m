#define VK_USE_PLATFORM_METAL_EXT
#import <Cocoa/Cocoa.h>
#import <QuartzCore/CAMetalLayer.h>
#include <vulkan/vulkan.h>
#include <stdlib.h>
#include <string.h>
#include "vk_platform.h"

bool vk_platform_is_kosmickrisp(void) {
    const char* icd = getenv("VK_ICD_FILENAMES");
    return icd && strstr(icd, "kosmickrisp") != NULL;
}

const char** vk_platform_instance_extensions(uint32_t* out_count) {
    if (vk_platform_is_kosmickrisp()) {
        static const char* exts[] = {
            VK_KHR_SURFACE_EXTENSION_NAME,
            VK_EXT_METAL_SURFACE_EXTENSION_NAME,
            "VK_KHR_get_physical_device_properties2"
        };
        *out_count = sizeof(exts) / sizeof(exts[0]);
        return exts;
    }
    static const char* exts[] = {
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_EXT_METAL_SURFACE_EXTENSION_NAME,
        VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME,
        "VK_KHR_get_physical_device_properties2"
    };
    *out_count = sizeof(exts) / sizeof(exts[0]);
    return exts;
}

VkInstanceCreateFlags vk_platform_instance_flags(void) {
    if (vk_platform_is_kosmickrisp()) return 0;
    return VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
}

const char** vk_platform_device_extensions(uint32_t* out_count) {
    if (vk_platform_is_kosmickrisp()) {
        static const char* exts[] = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };
        *out_count = sizeof(exts) / sizeof(exts[0]);
        return exts;
    }
    static const char* exts[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        "VK_KHR_portability_subset",
        "VK_EXT_metal_objects"
    };
    *out_count = sizeof(exts) / sizeof(exts[0]);
    return exts;
}

bool vk_platform_create_surface(VkInstance instance,
                                const Window* window,
                                VkSurfaceKHR* out_surface) {
    extern void* window_cocoa_get_metal_layer(const Window* w);
    CAMetalLayer* layer = (__bridge CAMetalLayer*)window_cocoa_get_metal_layer(window);

    VkMetalSurfaceCreateInfoEXT info = {0};
    info.sType  = VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT;
    info.pLayer = layer;
    return vkCreateMetalSurfaceEXT(instance, &info, NULL, out_surface) == VK_SUCCESS;
}