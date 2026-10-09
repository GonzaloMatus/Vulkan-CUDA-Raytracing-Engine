#include "command_buffers.h"
#include "../vulkan/vk_context.h"
#include "../swapchain/swapchain.h"
#include "../render_target/render_target.h"
#include <stdlib.h>
#include <stdio.h>

bool command_buffers_create(CommandBuffers* cmd, const VulkanContext* ctx, const Swapchain* swapchain) {
    cmd->count = swapchain->image_count;

    /* ---------------- Command Pool ---------------- */

    VkCommandPoolCreateInfo pool_info = {0};
    pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.queueFamilyIndex = ctx->graphics_queue_family;
    pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    if (vkCreateCommandPool(ctx->device, &pool_info, NULL, &cmd->pool) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create command pool\n");
        return false;
    }

    /* ---------------- Command Buffers ---------------- */

    cmd->buffers = malloc(sizeof(VkCommandBuffer) * cmd->count);

    VkCommandBufferAllocateInfo alloc = {0};
    alloc.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc.commandPool = cmd->pool;
    alloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc.commandBufferCount = cmd->count;

    if (vkAllocateCommandBuffers(ctx->device, &alloc, cmd->buffers) != VK_SUCCESS) {
        fprintf(stderr, "Failed to allocate command buffers\n");
        return false;
    }

    printf("Command buffers created (%u)\n", cmd->count);
    return true;
}

static void transition_image(VkCommandBuffer cmd, VkImage image, VkImageLayout old_layout, VkImageLayout new_layout, VkAccessFlags src_access, VkAccessFlags dst_access, VkPipelineStageFlags src_stage, VkPipelineStageFlags dst_stage) {
    VkImageMemoryBarrier barrier = {0};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = old_layout;
    barrier.newLayout = new_layout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.srcAccessMask = src_access;
    barrier.dstAccessMask = dst_access;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.layerCount = 1;

    vkCmdPipelineBarrier(cmd, src_stage, dst_stage, 0, 0, NULL, 0, NULL, 1, &barrier);
}

bool command_buffers_record(
    CommandBuffers* cmd,
    const VulkanContext* ctx,
    const Swapchain* swapchain,
    VkBuffer cuda_buffer,
    const RenderTargetHDR* hdr,
    const RenderTarget* ldr,
    const TonemapPipeline* tm
) {
    for (uint32_t i = 0; i < cmd->count; i++) {

        VkCommandBufferBeginInfo begin = {0};
        begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        vkBeginCommandBuffer(cmd->buffers[i], &begin);

        /* ============================================================
           1. HDR image: UNDEFINED → GENERAL (para copy buffer → image)
           ============================================================ */
        transition_image(
            cmd->buffers[i],
            hdr->image,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_GENERAL,
            0,
            VK_ACCESS_TRANSFER_WRITE_BIT,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT
        );

        /* ============================================================
           2. Copy CUDA buffer → HDR image
           ============================================================ */
        VkBufferImageCopy buffer_to_hdr = {0};
        buffer_to_hdr.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        buffer_to_hdr.imageSubresource.mipLevel = 0;
        buffer_to_hdr.imageSubresource.baseArrayLayer = 0;
        buffer_to_hdr.imageSubresource.layerCount = 1;

        buffer_to_hdr.imageOffset = (VkOffset3D){0, 0, 0};
        buffer_to_hdr.imageExtent = (VkExtent3D){
            hdr->width,
            hdr->height,
            1
        };

        vkCmdCopyBufferToImage(
            cmd->buffers[i],
            cuda_buffer,
            hdr->image,
            VK_IMAGE_LAYOUT_GENERAL,
            1,
            &buffer_to_hdr
        );

        /* ============================================================
           3. Barrier: transfer → compute (HDR read)
           ============================================================ */
        transition_image(
            cmd->buffers[i],
            hdr->image,
            VK_IMAGE_LAYOUT_GENERAL,
            VK_IMAGE_LAYOUT_GENERAL,
            VK_ACCESS_TRANSFER_WRITE_BIT,
            VK_ACCESS_SHADER_READ_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT
        );

        /* ============================================================
           4. LDR image: UNDEFINED → GENERAL (compute write)
           ============================================================ */
        transition_image(
            cmd->buffers[i],
            ldr->image,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_GENERAL,
            0,
            VK_ACCESS_SHADER_WRITE_BIT,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT
        );

        /* ============================================================
           5. Dispatch tonemap compute shader
           ============================================================ */
        vkCmdBindPipeline(
            cmd->buffers[i],
            VK_PIPELINE_BIND_POINT_COMPUTE,
            tm->pipeline
        );

        vkCmdBindDescriptorSets(
            cmd->buffers[i],
            VK_PIPELINE_BIND_POINT_COMPUTE,
            tm->pipeline_layout,
            0,
            1,
            &tm->desc_set,
            0,
            NULL
        );

        vkCmdDispatch(
            cmd->buffers[i],
            (hdr->width  + 15) / 16,
            (hdr->height + 15) / 16,
            1
        );

        /* ============================================================
           6. Barrier: compute → transfer (LDR read)
           ============================================================ */
        transition_image(
            cmd->buffers[i],
            ldr->image,
            VK_IMAGE_LAYOUT_GENERAL,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            VK_ACCESS_SHADER_WRITE_BIT,
            VK_ACCESS_TRANSFER_READ_BIT,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT
        );

        /* ============================================================
           7. Swapchain: UNDEFINED → TRANSFER_DST
           ============================================================ */
        transition_image(
            cmd->buffers[i],
            swapchain->images[i],
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            0,
            VK_ACCESS_TRANSFER_WRITE_BIT,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT
        );

        /* ============================================================
           8. Blit LDR image → swapchain.

           Usamos vkCmdBlitImage en lugar de vkCmdCopyImage porque el LDR
           y el swapchain pueden tener formatos con distinto orden de
           canales (RGBA8 vs BGRA8). El blit deja que el driver haga el
           swizzle de bytes si es necesario; el copy exigiría formatos
           idénticos byte a byte.

           - Windows: swapchain suele ser R8G8B8A8_UNORM. Blit RGBA8→RGBA8
             a tamaño idéntico es un copy 1:1.
           - macOS:  swapchain es B8G8R8A8_UNORM (CoreAnimation). Blit
             RGBA8→BGRA8 hace el swap R↔B de forma transparente.

           El filtro es NEAREST porque no hay escalado: src y dst tienen
           exactamente las mismas dimensiones.
           ============================================================ */
        VkImageBlit blit_region = {0};
        blit_region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit_region.srcSubresource.mipLevel   = 0;
        blit_region.srcSubresource.baseArrayLayer = 0;
        blit_region.srcSubresource.layerCount = 1;
        blit_region.srcOffsets[0] = (VkOffset3D){ 0, 0, 0 };
        blit_region.srcOffsets[1] = (VkOffset3D){
            (int32_t)ldr->width,
            (int32_t)ldr->height,
            1
        };

        blit_region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit_region.dstSubresource.mipLevel   = 0;
        blit_region.dstSubresource.baseArrayLayer = 0;
        blit_region.dstSubresource.layerCount = 1;
        blit_region.dstOffsets[0] = (VkOffset3D){ 0, 0, 0 };
        blit_region.dstOffsets[1] = (VkOffset3D){
            (int32_t)swapchain->extent.width,
            (int32_t)swapchain->extent.height,
            1
        };

        vkCmdBlitImage(
            cmd->buffers[i],
            ldr->image,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            swapchain->images[i],
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &blit_region,
            VK_FILTER_NEAREST
        );

        /* ============================================================
           9. Swapchain: TRANSFER_DST → PRESENT
           ============================================================ */
        transition_image(
            cmd->buffers[i],
            swapchain->images[i],
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            VK_ACCESS_TRANSFER_WRITE_BIT,
            VK_ACCESS_MEMORY_READ_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT
        );

        vkEndCommandBuffer(cmd->buffers[i]);
    }

    printf("Command buffers recorded (compute -> HDR -> Tonemap -> Swapchain)\n");
    return true;
}


void command_buffers_destroy(CommandBuffers* cmd, const VulkanContext* ctx) {
    vkFreeCommandBuffers(ctx->device, cmd->pool, cmd->count, cmd->buffers);

    vkDestroyCommandPool(ctx->device, cmd->pool, NULL);

    free(cmd->buffers);
    cmd->buffers = NULL;
    cmd->count = 0;
}