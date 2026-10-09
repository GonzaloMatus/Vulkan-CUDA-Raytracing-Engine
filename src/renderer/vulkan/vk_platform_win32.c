#define VK_USE_PLATFORM_WIN32_KHR
#include <windows.h>
#include <vulkan/vulkan.h>
#include "vk_platform.h"

const char** vk_platform_instance_extensions(uint32_t* out_count) {
    static const char* exts[] = {
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME
    };
    *out_count = sizeof(exts) / sizeof(exts[0]);
    return exts;
}

VkInstanceCreateFlags vk_platform_instance_flags(void) { return 0; }

const char** vk_platform_device_extensions(uint32_t* out_count) {
    static const char* exts[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME,
        VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME
    };
    *out_count = sizeof(exts) / sizeof(exts[0]);
    return exts;
}

bool vk_platform_create_surface(VkInstance instance,
                                const Window* window,
                                VkSurfaceKHR* out_surface) {
    VkWin32SurfaceCreateInfoKHR info = {0};
    info.sType     = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    info.hinstance = (HINSTANCE)window->instance;
    info.hwnd      = (HWND)window->handle;
    return vkCreateWin32SurfaceKHR(instance, &info, NULL, out_surface) == VK_SUCCESS;
}