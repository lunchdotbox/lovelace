#ifndef ENGINE_GRAPHICS_UNIFORM_H
#define ENGINE_GRAPHICS_UNIFORM_H

#include "buffer.h"
#include "device.h"
#include <elc/core.h>
#include <vulkan/vulkan_core.h>

typedef struct UniformBuffer {
    ValidBuffer buffer; // TODO: make a macro to generate structs with multiple buffers per memory
    void* mapped; // TODO: make sure its fine that this only has one buffer
} UniformBuffer;

UniformBuffer createUniformBuffer(Device device, VkDeviceSize size);
void destroyUniformBuffer(Device device, UniformBuffer uniform);
void updateUniformBuffer(Device device, UniformBuffer uniform, void* data, VkDeviceSize size);

#endif
