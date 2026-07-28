#pragma once
#include <vulkan/vulkan.h>
#include <stdint.h>
#include <windows.h>
#include <stdbool.h>

typedef struct {
    VkBuffer buffer;
    VkDeviceMemory memory;
    VkDeviceSize size;

    HANDLE win32_handle;
} GpuRenderBuffer;

bool gpu_render_buffer_create(
    GpuRenderBuffer* grb,
    VkPhysicalDevice physical_device,
    VkDevice device,
    uint32_t width,
    uint32_t height
);

void gpu_render_buffer_destroy(
    GpuRenderBuffer* grb,
    VkDevice device
);
