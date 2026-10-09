#pragma once
#include <vulkan/vulkan.h>
#include <stdint.h>
#include <stdbool.h>
#include "core/window.h"

const char** vk_platform_instance_extensions(uint32_t* out_count);
VkInstanceCreateFlags vk_platform_instance_flags(void);
const char** vk_platform_device_extensions(uint32_t* out_count);

bool vk_platform_create_surface(VkInstance instance,
                                const Window* window,
                                VkSurfaceKHR* out_surface);

// Devuelve true si el ICD activo es KosmicKrisp (conforme a Vulkan).
// En Windows siempre devuelve false.
bool vk_platform_is_kosmickrisp(void);