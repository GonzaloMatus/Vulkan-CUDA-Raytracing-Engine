#define VK_USE_PLATFORM_METAL_EXT
#import <Cocoa/Cocoa.h>
#import <QuartzCore/CAMetalLayer.h>
#include <vulkan/vulkan.h>
#include "vk_platform.h"

const char** vk_platform_instance_extensions(uint32_t* out_count) {
    static const char* exts[] = {
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_EXT_METAL_SURFACE_EXTENSION_NAME,
        VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME,  // requerido en macOS
        "VK_KHR_get_physical_device_properties2"
    };
    *out_count = sizeof(exts) / sizeof(exts[0]);
    return exts;
}

VkInstanceCreateFlags vk_platform_instance_flags(void) {
    return VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
}

const char** vk_platform_device_extensions(uint32_t* out_count) {
    static const char* exts[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        "VK_KHR_portability_subset",         // requerido por MoltenVK
        "VK_EXT_metal_objects"               // para interop con Metal
    };
    *out_count = sizeof(exts) / sizeof(exts[0]);
    return exts;
}

bool vk_platform_create_surface(VkInstance instance,
                                const Window* window,
                                VkSurfaceKHR* out_surface) {
    // window->handle es CocoaState* en Mac
    extern void* window_cocoa_get_metal_layer(const Window* w);
    CAMetalLayer* layer = (__bridge CAMetalLayer*)window_cocoa_get_metal_layer(window);

    VkMetalSurfaceCreateInfoEXT info = {0};
    info.sType  = VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT;
    info.pLayer = layer;
    return vkCreateMetalSurfaceEXT(instance, &info, NULL, out_surface) == VK_SUCCESS;
}