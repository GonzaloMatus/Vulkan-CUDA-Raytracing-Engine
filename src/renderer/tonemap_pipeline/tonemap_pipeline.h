#pragma once
#include <vulkan/vulkan.h>
#include <stdint.h>
#include <windows.h>
#include <stdbool.h>
#include "../vulkan/vk_context.h"

typedef struct {
    VkPipelineLayout pipeline_layout;
    VkPipeline pipeline;
    VkDescriptorSetLayout desc_layout;
    VkDescriptorPool desc_pool;
    VkDescriptorSet desc_set;
} TonemapPipeline;

bool tonemap_pipeline_create(TonemapPipeline* tm, const VulkanContext* ctx, VkImageView hdr_view, VkImageView ldr_view);

void tonemap_pipeline_destroy(TonemapPipeline* tm, const VulkanContext* ctx);
