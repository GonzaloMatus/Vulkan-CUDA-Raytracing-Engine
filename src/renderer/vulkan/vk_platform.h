// vk_platform.h
#pragma once
#include <vulkan/vulkan.h>
#include <stdint.h>
#include <stdbool.h>
#include "core/window.h"

// Devuelve el array de extensiones de instancia específicas de plataforma.
// El caller no debe liberarlo.
const char** vk_platform_instance_extensions(uint32_t* out_count);

// Flags extra para VkInstanceCreateInfo (p. ej. portability en macOS).
VkInstanceCreateFlags vk_platform_instance_flags(void);

// Devuelve el array de extensiones de dispositivo específicas de plataforma.
const char** vk_platform_device_extensions(uint32_t* out_count);

// Crea la VkSurfaceKHR a partir de los handles nativos en window.
bool vk_platform_create_surface(VkInstance instance,
                                const Window* window,
                                VkSurfaceKHR* out_surface);