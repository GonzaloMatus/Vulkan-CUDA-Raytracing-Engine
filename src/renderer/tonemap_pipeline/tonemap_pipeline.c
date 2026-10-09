#include "tonemap_pipeline.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static VkShaderModule load_shader_module(VkDevice device, const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "Failed to open shader %s\n", path);
        return VK_NULL_HANDLE;
    }

    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    rewind(f);

    char* code = malloc(size);
    fread(code, 1, size, f);
    fclose(f);

    VkShaderModuleCreateInfo info = {0};
    info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    info.codeSize = size;
    info.pCode = (const uint32_t*)code;

    VkShaderModule module;
    if (vkCreateShaderModule(device, &info, NULL, &module) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create shader module\n");
        free(code);
        return VK_NULL_HANDLE;
    }

    free(code);
    return module;
}


bool tonemap_pipeline_create(TonemapPipeline* tm, const VulkanContext* ctx, VkImageView hdr_view, VkImageView ldr_view) {
    memset(tm, 0, sizeof(*tm));

    /* ============================================================
       Descriptor set layout
       ============================================================ */

    VkDescriptorSetLayoutBinding bindings[2] = {0};

    bindings[0].binding = 0;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    bindings[0].descriptorCount = 1;
    bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    bindings[1].binding = 1;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    bindings[1].descriptorCount = 1;
    bindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutCreateInfo layout_info = {0};
    layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_info.bindingCount = 2;
    layout_info.pBindings = bindings;

    if (vkCreateDescriptorSetLayout(ctx->device, &layout_info, NULL, &tm->desc_layout) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create tonemap descriptor layout\n");
        return false;
    }

    /* ============================================================
       Descriptor pool
       ============================================================ */

    VkDescriptorPoolSize pool_size = {0};
    pool_size.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    pool_size.descriptorCount = 2;

    VkDescriptorPoolCreateInfo pool_info = {0};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.poolSizeCount = 1;
    pool_info.pPoolSizes = &pool_size;
    pool_info.maxSets = 1;

    if (vkCreateDescriptorPool(ctx->device, &pool_info, NULL, &tm->desc_pool) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create tonemap descriptor pool\n");
        return false;
    }

    /* ============================================================
       Descriptor set
       ============================================================ */

    VkDescriptorSetAllocateInfo alloc_info = {0};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorPool = tm->desc_pool;
    alloc_info.descriptorSetCount = 1;
    alloc_info.pSetLayouts = &tm->desc_layout;

    if (vkAllocateDescriptorSets(ctx->device, &alloc_info, &tm->desc_set) != VK_SUCCESS) {
        fprintf(stderr, "Failed to allocate tonemap descriptor set\n");
        return false;
    }

    VkDescriptorImageInfo hdr_info = {0};
    hdr_info.imageView = hdr_view;
    hdr_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

    VkDescriptorImageInfo ldr_info = {0};
    ldr_info.imageView = ldr_view;
    ldr_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

    VkWriteDescriptorSet writes[2] = {0};

    writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[0].dstSet = tm->desc_set;
    writes[0].dstBinding = 0;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    writes[0].descriptorCount = 1;
    writes[0].pImageInfo = &hdr_info;

    writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[1].dstSet = tm->desc_set;
    writes[1].dstBinding = 1;
    writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    writes[1].descriptorCount = 1;
    writes[1].pImageInfo = &ldr_info;

    vkUpdateDescriptorSets(ctx->device, 2, writes, 0, NULL);

    /* ============================================================
       Pipeline layout
       ============================================================ */

    VkPipelineLayoutCreateInfo pipeline_layout_info = {0};
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.setLayoutCount = 1;
    pipeline_layout_info.pSetLayouts = &tm->desc_layout;

    if (vkCreatePipelineLayout(ctx->device, &pipeline_layout_info, NULL, &tm->pipeline_layout) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create tonemap pipeline layout\n");
        return false;
    }

    /* ============================================================
       Compute pipeline
       ============================================================ */

    VkShaderModule shader = load_shader_module(ctx->device, "shaders/tonemap.spv");

    if (shader == VK_NULL_HANDLE) {
        return false;
    }

    VkPipelineShaderStageCreateInfo stage = {0};
    stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    stage.module = shader;
    stage.pName = "main";

    VkComputePipelineCreateInfo pipeline_info = {0};
    pipeline_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipeline_info.stage = stage;
    pipeline_info.layout = tm->pipeline_layout;

    if (vkCreateComputePipelines(ctx->device, VK_NULL_HANDLE, 1, &pipeline_info, NULL, &tm->pipeline) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create tonemap compute pipeline\n");
        vkDestroyShaderModule(ctx->device, shader, NULL);
        return false;
    }

    vkDestroyShaderModule(ctx->device, shader, NULL);

    printf("Tonemap compute pipeline created\n");
    return true;
}


void tonemap_pipeline_destroy(TonemapPipeline* tm, const VulkanContext* ctx) {
    if (tm->pipeline) {
        vkDestroyPipeline(ctx->device, tm->pipeline, NULL);
    }

    if (tm->pipeline_layout) {
        vkDestroyPipelineLayout(ctx->device, tm->pipeline_layout, NULL);
    }

    if (tm->desc_pool) {
        vkDestroyDescriptorPool(ctx->device, tm->desc_pool, NULL);
    }

    if (tm->desc_layout) {
        vkDestroyDescriptorSetLayout(ctx->device, tm->desc_layout, NULL);
    }

    memset(tm, 0, sizeof(*tm));
}
