#ifndef ENGINE_GRAPHICS_BUFFER_H
#define ENGINE_GRAPHICS_BUFFER_H

#include "device.h"

typedef struct ValidBuffer {
    VkBuffer buffer;
    VkDeviceMemory memory;
} ValidBuffer;

VkBuffer createBuffer(Device device, VkDeviceSize size, VkBufferUsageFlags usage, bool concurrent);
VkDeviceMemory createBuffersMemory(Device device, VkBuffer* buffers, u32 n_buffers, VkMemoryPropertyFlags properties);
void copyBuffer(Device device, QueueType type, VkBuffer src_buffer, VkBuffer dst_buffer, u64 copy_size);
ValidBuffer createValidBuffer(Device device, VkDeviceSize size, VkBufferUsageFlags usage, bool concurrent, VkMemoryPropertyFlags properties);
void destroyValidBuffer(Device device, ValidBuffer buffer);
void makePermanentBuffers(Device device, QueueType type, u32 n_buffers, ValidBuffer* buffers, u64* sizes, VkBufferUsageFlags* usages, bool* concurrents);

#endif
