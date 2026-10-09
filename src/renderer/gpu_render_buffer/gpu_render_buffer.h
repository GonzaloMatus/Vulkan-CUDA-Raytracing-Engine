#pragma once
#include <vulkan/vulkan.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    VkBuffer       buffer;
    VkDeviceMemory memory;
    VkDeviceSize   size;

    // Puntero host al buffer mapeado. Solo válido en plataformas UMA
    // (macOS Apple Silicon). En Windows es NULL.
    void*          mapped;

    // Handle nativo para interop con el backend de cómputo.
    // Win32: HANDLE de VK_KHR_external_memory_win32
    // macOS: mismo valor que `mapped` (para que raytracer lo use directo)
    void*          native_handle;
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