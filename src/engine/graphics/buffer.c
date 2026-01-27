#include "buffer.h"

#include "command.h"
#include "device.h"
#include <vulkan/vulkan_core.h>

VkBuffer createBuffer(Device device, VkDeviceSize size, VkBufferUsageFlags usage, bool concurrent) {
    VkBufferCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = usage,
        .sharingMode = concurrent ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE,
    };

    VkBuffer buffer;
    vkCreateBuffer(device.logical, &create_info, NULL, &buffer);

    return buffer;
}

VkDeviceMemory createBuffersMemory(Device device, VkBuffer* buffers, u32 n_buffers, VkMemoryPropertyFlags properties) {
    VkMemoryRequirements requirements;
    vkGetBufferMemoryRequirements(device.logical, *buffers, &requirements);
    requirements.size *= n_buffers;

    VkDeviceMemory memory = allocateDeviceMemory(device, properties, requirements);
    for (u32 i = 0; i < n_buffers; i++) vkBindBufferMemory(device.logical, buffers[i], memory, i * (requirements.size / n_buffers));

    return memory;
}

void copyBuffer(Device device, QueueType type, VkBuffer src_buffer, VkBuffer dst_buffer, u64 copy_size) {
    VkCommandBuffer command = beginSingleTimeCommands(device, type);

    commandCopyBuffer(command, src_buffer, dst_buffer, copy_size);

    endSingleTimeCommands(device, type, command);
}

ValidBuffer createValidBuffer(Device device, VkDeviceSize size, VkBufferUsageFlags usage, bool concurrent, VkMemoryPropertyFlags properties) {
    ValidBuffer buffer;
    buffer.buffer = createBuffer(device, size, usage, concurrent);
    buffer.memory = createBuffersMemory(device, &buffer.buffer, 1, properties);
    return buffer;
}

void destroyValidBuffer(Device device, ValidBuffer buffer) {
    vkFreeMemory(device.logical, buffer.memory, NULL);
    vkDestroyBuffer(device.logical, buffer.buffer, NULL);
}

void makePermanentBuffers(Device device, QueueType type, u32 n_buffers, ValidBuffer* buffers, u64* sizes, VkBufferUsageFlags* usages, bool* concurrents) {
    for (u32 i = 0; i < n_buffers; i++) vkUnmapMemory(device.logical, buffers[i].memory);

    ValidBuffer new_buffers[n_buffers];
    for (u32 i = 0; i < n_buffers; i++) new_buffers[i] = createValidBuffer(device, sizes[i], usages[i], concurrents[i], VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    VkCommandBuffer command = beginSingleTimeCommands(device, type);
    for (u32 i = 0; i < n_buffers; i++) commandCopyBuffer(command, buffers[i].buffer, new_buffers[i].buffer, sizes[i]);
    endSingleTimeCommands(device, type, command);

    for (u32 i = 0; i < n_buffers; i++) destroyValidBuffer(device, buffers[i]);
    for (u32 i = 0; i < n_buffers; i++) buffers[i] = new_buffers[i];
}
