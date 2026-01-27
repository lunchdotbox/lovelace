#ifndef ENGINE_GRAPHICS_COMMAND_H
#define ENGINE_GRAPHICS_COMMAND_H

#include "device.h"

VkCommandPool createCommandPool(Device device, u32 queue_family);
void freeCommandBuffers(Device device, QueueType type, u32 count, VkCommandBuffer* result);
void allocateCommandBuffers(Device device, QueueType type, u32 count, VkCommandBuffer* result);
void beginCommandBuffer(VkCommandBuffer buffer);
VkCommandBuffer beginSingleTimeCommands(Device device, QueueType type);
void endSingleTimeCommands(Device device, QueueType type, VkCommandBuffer command_buffer);
void commandCopyBuffer(VkCommandBuffer command_buffer, VkBuffer src_buffer, VkBuffer dst_buffer, u64 copy_size);
void commandCopyBufferToImage(VkCommandBuffer command, VkExtent3D extent, VkBuffer buffer, VkImage image);
void imageMemoryBarrier(VkCommandBuffer command, VkImageLayout old, VkImageLayout new, VkAccessFlags src_mask, VkAccessFlags dst_mask, VkPipelineStageFlags src_stage, VkPipelineStageFlags dst_stage, u32 layers, VkImage image);
void commandPushConstants(VkCommandBuffer command, Device device, VkShaderStageFlagBits stage, u32 size, const void* values);

#endif
