#include "uniform.h"

#include "buffer.h"
#include "descriptor.h"
#include "device.h"
#include "device_loop.h"

#include <string.h>
#include <vulkan/vulkan_core.h>

UniformBuffer createUniformBuffer(Device device, VkDeviceSize size) {
    UniformBuffer uniform;
    uniform.buffer = createValidBuffer(device, size * FRAMES_IN_FLIGHT, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, false, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    uniform.mapped = mapDeviceMemory(device, uniform.buffer.memory, 0, size * FRAMES_IN_FLIGHT);
    return uniform;
}

void destroyUniformBuffer(Device device, UniformBuffer uniform) {
    vkUnmapMemory(device.logical, uniform.buffer.memory);
    destroyValidBuffer(device, uniform.buffer);
}

void updateUniformBuffer(Device device, DeviceLoop loop, UniformBuffer uniform, void* data, VkDeviceSize size) {
    memcpy(uniform.mapped + (loop.frame * size), data, size);
    flushDeviceMemory(device, uniform.buffer.memory, loop.frame * size, size);
}

void registerUniformBuffer(Device device, DeviceLoop* loop, UniformBuffer uniform, VkDeviceSize size, u32* ids) {
    for (u32 i = 0; i < FRAMES_IN_FLIGHT; i++) ids[i] = addDescriptorUniformBuffer(device, loop, uniform.buffer.buffer, size * i, size);
}
