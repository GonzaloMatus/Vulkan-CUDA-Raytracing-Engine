#include <vulkan/vulkan.h>

#include "gpu_render_buffer.h"
#include <string.h>
#include <stdio.h>

#if defined(_WIN32)
#  define VK_USE_PLATFORM_WIN32_KHR
#  include <windows.h>
#  include <vulkan/vulkan_win32.h>
#endif

static uint32_t find_memory_type(VkPhysicalDevice physical_device,
                                 uint32_t type_filter,
                                 VkMemoryPropertyFlags properties) {
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

bool gpu_render_buffer_create(GpuRenderBuffer* grb,
                              VkPhysicalDevice physical_device,
                              VkDevice device,
                              uint32_t width, uint32_t height) {
    memset(grb, 0, sizeof(*grb));

    const size_t size_of_half = 2;
    grb->size = (VkDeviceSize)width * height * size_of_half * 4;

    VkBufferCreateInfo buffer_info = {0};
    buffer_info.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size        = grb->size;
    buffer_info.usage       = VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
                              VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

#if defined(_WIN32)
    VkExternalMemoryBufferCreateInfo external_buffer_info = {0};
    external_buffer_info.sType       = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_BUFFER_CREATE_INFO;
    external_buffer_info.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;
    buffer_info.pNext = &external_buffer_info;
#endif

    if (vkCreateBuffer(device, &buffer_info, NULL, &grb->buffer) != VK_SUCCESS) {
        printf("Failed to create GPU render buffer\n");
        return false;
    }

    VkMemoryRequirements mem_req;
    vkGetBufferMemoryRequirements(device, grb->buffer, &mem_req);

    // Windows: DEVICE_LOCAL para interop con CUDA (rápido).
    // macOS: HOST_VISIBLE para poder mapear directamente (UMA, sin copias).
#if defined(_WIN32)
    VkMemoryPropertyFlags mem_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
#else
    VkMemoryPropertyFlags mem_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
#endif

    uint32_t memory_type = find_memory_type(physical_device, mem_req.memoryTypeBits, mem_flags);

    if (memory_type == UINT32_MAX) {
        printf("No suitable memory type found (flags=0x%x)\n", mem_flags);
        return false;
    }

    VkMemoryAllocateInfo alloc = {0};
    alloc.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc.allocationSize  = mem_req.size;
    alloc.memoryTypeIndex = memory_type;

#if defined(_WIN32)
    VkExportMemoryAllocateInfo export_info = {0};
    export_info.sType       = VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO;
    export_info.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;
    alloc.pNext = &export_info;
#endif

    if (vkAllocateMemory(device, &alloc, NULL, &grb->memory) != VK_SUCCESS) {
        printf("Failed to allocate GPU render buffer memory\n");
        return false;
    }

    vkBindBufferMemory(device, grb->buffer, grb->memory, 0);

#if defined(_WIN32)
    // Windows: exportar handle para CUDA
    VkMemoryGetWin32HandleInfoKHR handle_info = {0};
    handle_info.sType      = VK_STRUCTURE_TYPE_MEMORY_GET_WIN32_HANDLE_INFO_KHR;
    handle_info.memory     = grb->memory;
    handle_info.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;

    PFN_vkGetMemoryWin32HandleKHR vkGetMemoryWin32HandleKHR =
        (PFN_vkGetMemoryWin32HandleKHR) vkGetDeviceProcAddr(device, "vkGetMemoryWin32HandleKHR");

    if (!vkGetMemoryWin32HandleKHR ||
        vkGetMemoryWin32HandleKHR(device, &handle_info, (HANDLE*)&grb->native_handle) != VK_SUCCESS) {
        printf("Failed to get Win32 memory handle\n");
        return false;
    }
#else
    // macOS: mapear el buffer a memoria del host. En Apple Silicon con UMA
    // el mapeo es directo, sin copias.
    if (vkMapMemory(device, grb->memory, 0, grb->size, 0, &grb->mapped) != VK_SUCCESS) {
        printf("Failed to map GPU render buffer memory\n");
        return false;
    }
    // El "handle" que pasamos al backend de cómputo es el mapping directo.
    grb->native_handle = grb->mapped;
#endif

    printf("GPU render buffer created (%llu bytes)\n", (unsigned long long)grb->size);
    return true;
}

void gpu_render_buffer_destroy(GpuRenderBuffer* grb, VkDevice device) {
    if (grb->mapped) {
        vkUnmapMemory(device, grb->memory);
        grb->mapped = NULL;
    }
    if (grb->buffer) vkDestroyBuffer(device, grb->buffer, NULL);
    if (grb->memory) vkFreeMemory(device, grb->memory, NULL);
    memset(grb, 0, sizeof(*grb));
}