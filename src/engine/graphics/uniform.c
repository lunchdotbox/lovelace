#include "uniform.h"

#include "buffer.h"
#include "device.h"
#include <string.h>
#include <vulkan/vulkan_core.h>

UniformBuffer createUniformBuffer(Device device, VkDeviceSize size) {
    UniformBuffer uniform;
    uniform.buffer = createValidBuffer(device, size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, false, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    uniform.mapped = mapDeviceMemory(device, uniform.buffer.memory, 0, size);
    return uniform;
}

void destroyUniformBuffer(Device device, UniformBuffer uniform) {
    vkUnmapMemory(device.logical, uniform.buffer.memory);
    destroyValidBuffer(device, uniform.buffer);
}

void updateUniformBuffer(Device device, UniformBuffer uniform, void* data, VkDeviceSize size) {
    memcpy(uniform.mapped, data, size);
    flushDeviceMemory(device, uniform.buffer.memory, 0, VK_WHOLE_SIZE);
}
