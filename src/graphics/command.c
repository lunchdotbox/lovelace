#include "command.h"

#include "device.h"
#include <vulkan/vulkan_core.h>

VkCommandPool createCommandPool(Device device, u32 queue_family) {
    VkCommandPoolCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = queue_family,
    };

    VkCommandPool command_pool;
    vkCreateCommandPool(device.logical, &info, NULL, &command_pool);

    return command_pool;
}

void freeCommandBuffers(Device device, QueueType type, u32 count, VkCommandBuffer* result) {
    vkFreeCommandBuffers(device.logical, queueTypeToPool(device, type), count, result);
}

void allocateCommandBuffers(Device device, QueueType type, u32 count, VkCommandBuffer* result) {
    VkCommandBufferAllocateInfo info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandPool = queueTypeToPool(device, type),
        .commandBufferCount = count,
    };

    vkAllocateCommandBuffers(device.logical, &info, result);
}

void beginCommandBuffer(VkCommandBuffer buffer) {
    VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    };

    vkBeginCommandBuffer(buffer, &begin_info);
}

VkCommandBuffer beginSingleTimeCommands(Device device, QueueType type) {
    VkCommandBuffer command;
    allocateCommandBuffers(device, type, 1, &command);

    VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };

    vkBeginCommandBuffer(command, &begin_info);

    return command;
}

void endSingleTimeCommands(Device device, QueueType type, VkCommandBuffer command_buffer) {
    vkEndCommandBuffer(command_buffer);

    VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &command_buffer,
    };

    vkQueueSubmit(queueTypeToQueue(device, type), 1, &submit_info, VK_NULL_HANDLE);
    vkQueueWaitIdle(queueTypeToQueue(device, type));

    vkFreeCommandBuffers(device.logical, queueTypeToPool(device, type), 1, &command_buffer);
}

void commandCopyBuffer(VkCommandBuffer command_buffer, VkBuffer src_buffer, VkBuffer dst_buffer, u64 copy_size) {
    VkBufferCopy copy_region = {.size = copy_size};
    vkCmdCopyBuffer(command_buffer, src_buffer, dst_buffer, 1, &copy_region);
}

void commandCopyBufferToImage(VkCommandBuffer command, VkExtent3D extent, VkBuffer buffer, VkImage image) {
    VkBufferImageCopy region = {
        .imageSubresource = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
        .imageExtent = extent,
    };

    vkCmdCopyBufferToImage(command, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}

void imageMemoryBarrier(VkCommandBuffer command, VkImageLayout old, VkImageLayout new, VkAccessFlags src_mask, VkAccessFlags dst_mask, VkPipelineStageFlags src_stage, VkPipelineStageFlags dst_stage, u32 layers, VkImage image) {
    VkImageMemoryBarrier barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .oldLayout = old,
        .newLayout = new,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image,
        .srcAccessMask = src_mask,
        .dstAccessMask = dst_mask,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = layers,
        },
    };

    vkCmdPipelineBarrier(command, src_stage, dst_stage, 0, 0, NULL, 0, NULL, 1, &barrier);
}

void commandPushConstants(VkCommandBuffer command, Device device, VkShaderStageFlagBits stage, u32 size, const void* values) {
    vkCmdPushConstants(command, device.pipeline_layout, stage, 0, size, values);
}
