// vk_context.c
#include <vulkan/vulkan.h>
#include "vk_context.h"
#include "vk_platform.h"
#include <stdio.h>
#include <string.h>
#include "../../core/window.h"

#ifndef NDEBUG
static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT type,
    const VkDebugUtilsMessengerCallbackDataEXT* data,
    void* user_data)
{
    (void)type;
    (void)user_data;

    const char* level =
        (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)   ? "ERROR" :
        (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) ? "WARN"  :
        "INFO";

    fprintf(stderr, "[VULKAN %s] %s\n", level, data->pMessage);
    return VK_FALSE;
}

static VkResult create_debug_messenger(VkInstance instance,
                                       VkDebugUtilsMessengerEXT* messenger)
{
    VkDebugUtilsMessengerCreateInfoEXT info = {0};
    info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    info.messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    info.messageType =
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT     |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT  |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    info.pfnUserCallback = debug_callback;

    PFN_vkCreateDebugUtilsMessengerEXT func =
        (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(
            instance, "vkCreateDebugUtilsMessengerEXT");

    if (!func) return VK_ERROR_EXTENSION_NOT_PRESENT;
    return func(instance, &info, NULL, messenger);
}

static void destroy_debug_messenger(VkInstance instance,
                                    VkDebugUtilsMessengerEXT messenger)
{
    PFN_vkDestroyDebugUtilsMessengerEXT func =
        (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(
            instance, "vkDestroyDebugUtilsMessengerEXT");

    if (func) func(instance, messenger, NULL);
}
#endif /* !NDEBUG */


typedef struct {
    VkPhysicalDevice physical_device;

    uint32_t graphics_family;
    uint32_t present_family;

    bool has_graphics;
    bool has_present;
} VulkanDeviceSelection;

typedef struct {
    VkDevice device;
    VkQueue  graphics_queue;
    VkQueue  present_queue;
} VulkanDevice;


static bool vk_create_instance(VulkanContext* ctx) {
    memset(ctx, 0, sizeof(*ctx));

#ifndef NDEBUG
    ctx->enable_validation = true;
#endif

    VkApplicationInfo app_info = {0};
    app_info.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName   = "C Engine";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName        = "Custom Engine";
    app_info.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion         = VK_API_VERSION_1_2;

    VkInstanceCreateInfo create_info = {0};
    create_info.sType            = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &app_info;
    create_info.flags            = vk_platform_instance_flags();

    /* --- Extensiones de instancia: plataforma + debug opcional --- */
    const char* extensions[8];
    uint32_t extension_count = 0;

    uint32_t platform_ext_count = 0;
    const char** platform_exts  = vk_platform_instance_extensions(&platform_ext_count);
    for (uint32_t i = 0; i < platform_ext_count; i++) {
        extensions[extension_count++] = platform_exts[i];
    }

#ifndef NDEBUG
    extensions[extension_count++] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
#endif

    create_info.enabledExtensionCount   = extension_count;
    create_info.ppEnabledExtensionNames = extensions;

#ifndef NDEBUG
    static const char* validation_layers[] = {
        "VK_LAYER_KHRONOS_validation"
    };
    create_info.enabledLayerCount   = 1;
    create_info.ppEnabledLayerNames = validation_layers;
#else
    create_info.enabledLayerCount = 0;
#endif

    VkResult res = vkCreateInstance(&create_info, NULL, &ctx->instance);
    if (res != VK_SUCCESS) {
        fprintf(stderr, "Failed to create Vulkan instance (%d)\n", res);
        return false;
    }

#ifndef NDEBUG
    if (ctx->enable_validation) {
        if (create_debug_messenger(ctx->instance, &ctx->debug_messenger) != VK_SUCCESS) {
            fprintf(stderr, "Failed to create Vulkan debug messenger\n");
        } else {
            printf("Vulkan debug messenger created\n");
        }
    }
#endif

    printf("Vulkan instance created successfully\n");
    return true;
}


static bool vk_pick_physical_device(VkInstance instance,
                                    VkSurfaceKHR surface,
                                    VulkanDeviceSelection* out)
{
    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(instance, &device_count, NULL);

    if (device_count == 0) {
        fprintf(stderr, "No Vulkan-compatible GPUs found\n");
        return false;
    }

    VkPhysicalDevice devices[16];
    if (device_count > 16) device_count = 16;
    vkEnumeratePhysicalDevices(instance, &device_count, devices);

    for (uint32_t i = 0; i < device_count; i++) {
        VkPhysicalDevice device = devices[i];

        uint32_t queue_family_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, NULL);

        VkQueueFamilyProperties families[16];
        if (queue_family_count > 16) queue_family_count = 16;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, families);

        bool     found_graphics = false;
        bool     found_present  = false;
        uint32_t graphics_index = 0;
        uint32_t present_index  = 0;

        for (uint32_t j = 0; j < queue_family_count; j++) {
            if (families[j].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                found_graphics = true;
                graphics_index = j;
            }

            VkBool32 supports_present = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, j, surface, &supports_present);

            if (supports_present) {
                found_present = true;
                present_index = j;
            }
        }

        if (found_graphics && found_present) {
            out->physical_device = device;
            out->graphics_family = graphics_index;
            out->present_family  = present_index;
            out->has_graphics    = true;
            out->has_present     = true;

            VkPhysicalDeviceProperties props;
            vkGetPhysicalDeviceProperties(device, &props);
            printf("Selected GPU: %s\n", props.deviceName);
            return true;
        }
    }

    fprintf(stderr, "No suitable GPU found\n");
    return false;
}


static bool vk_create_logical_device(const VulkanDeviceSelection* selection,
                                     VulkanDevice* out_device)
{
    float queue_priority = 1.0f;

    VkDeviceQueueCreateInfo queue_infos[2];
    uint32_t queue_info_count = 0;

    queue_infos[queue_info_count] = (VkDeviceQueueCreateInfo){
        .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = selection->graphics_family,
        .queueCount       = 1,
        .pQueuePriorities = &queue_priority
    };
    queue_info_count++;

    if (selection->present_family != selection->graphics_family) {
        queue_infos[queue_info_count] = (VkDeviceQueueCreateInfo){
            .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = selection->present_family,
            .queueCount       = 1,
            .pQueuePriorities = &queue_priority
        };
        queue_info_count++;
    }

    VkPhysicalDeviceFeatures device_features = {0};

    VkDeviceCreateInfo create_info = {0};
    create_info.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    create_info.queueCreateInfoCount    = queue_info_count;
    create_info.pQueueCreateInfos       = queue_infos;
    create_info.pEnabledFeatures        = &device_features;

    uint32_t device_ext_count = 0;
    const char** device_exts  = vk_platform_device_extensions(&device_ext_count);
    create_info.enabledExtensionCount   = device_ext_count;
    create_info.ppEnabledExtensionNames = device_exts;

    VkResult res = vkCreateDevice(selection->physical_device,
                                  &create_info, NULL, &out_device->device);
    if (res != VK_SUCCESS) {
        fprintf(stderr, "Failed to create logical device (%d)\n", res);
        return false;
    }

    vkGetDeviceQueue(out_device->device, selection->graphics_family, 0,
                     &out_device->graphics_queue);
    vkGetDeviceQueue(out_device->device, selection->present_family, 0,
                     &out_device->present_queue);

    printf("Logical device and queues created successfully\n");
    return true;
}


bool vk_context_init(VulkanContext* ctx, const Window* window) {
    if (!vk_create_instance(ctx))
        return false;

    if (!vk_platform_create_surface(ctx->instance, window, &ctx->surface)) {
        fprintf(stderr, "Failed to create Vulkan surface\n");
        return false;
    }

    VulkanDeviceSelection selection = {0};
    if (!vk_pick_physical_device(ctx->instance, ctx->surface, &selection))
        return false;

    ctx->physical_device       = selection.physical_device;
    ctx->graphics_queue_family = selection.graphics_family;
    ctx->present_queue_family  = selection.present_family;

    VulkanDevice device = {0};
    if (!vk_create_logical_device(&selection, &device))
        return false;

    ctx->device         = device.device;
    ctx->graphics_queue = device.graphics_queue;
    ctx->present_queue  = device.present_queue;

    return true;
}


void vk_context_shutdown(VulkanContext* ctx) {
    if (ctx->device) {
        vkDeviceWaitIdle(ctx->device);
        vkDestroyDevice(ctx->device, NULL);
        ctx->device = VK_NULL_HANDLE;
    }

    if (ctx->surface) {
        vkDestroySurfaceKHR(ctx->instance, ctx->surface, NULL);
        ctx->surface = VK_NULL_HANDLE;
    }

#ifndef NDEBUG
    if (ctx->enable_validation && ctx->debug_messenger) {
        destroy_debug_messenger(ctx->instance, ctx->debug_messenger);
        ctx->debug_messenger = VK_NULL_HANDLE;
    }
#endif

    if (ctx->instance) {
        vkDestroyInstance(ctx->instance, NULL);
        ctx->instance = VK_NULL_HANDLE;
    }
}