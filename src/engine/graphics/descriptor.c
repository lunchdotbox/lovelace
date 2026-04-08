#include "descriptor.h"

#include "device.h"
#include "device_loop.h"
#include "texture.h"
#include "../utilities/array.h"

VkDescriptorPool createDescriptorPool(Device device, u32 set_count, u32 descriptor_count) {
    VkDescriptorPoolCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
        .pPoolSizes = (VkDescriptorPoolSize[]){
            {.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = set_count * descriptor_count},
            {.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .descriptorCount = set_count * descriptor_count},
            {.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .descriptorCount = set_count * descriptor_count},
        },
        .poolSizeCount = 3,
        .maxSets = set_count,
    };

    VkDescriptorPool descriptor_pool;
    vkCreateDescriptorPool(device.logical, &info, NULL, &descriptor_pool);

    return descriptor_pool;
}

VkDescriptorSetLayout createSetLayout(Device device, u32 descriptor_count) {
    VkDescriptorSetLayoutBinding bindings[] = {
        (VkDescriptorSetLayoutBinding){.binding = 0, .descriptorCount = descriptor_count, .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT},
        (VkDescriptorSetLayoutBinding){.binding = 1, .descriptorCount = descriptor_count, .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT},
        (VkDescriptorSetLayoutBinding){.binding = 2, .descriptorCount = descriptor_count, .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT},
    };

    VkDescriptorBindingFlags flags[] = {
        VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT,
        VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT,
        VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT,
    };

    VkDescriptorSetLayoutBindingFlagsCreateInfo flags_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
        .bindingCount = ARRAY_LENGTH(flags),
        .pBindingFlags = flags,
    };

    VkDescriptorSetLayoutCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = &flags_info,
        .bindingCount = ARRAY_LENGTH(bindings),
        .pBindings = bindings,
    };

    VkDescriptorSetLayout set_layout;
    vkCreateDescriptorSetLayout(device.logical, &info, NULL, &set_layout);

    return set_layout;
}

void allocateDescriptorSets(Device device, VkDescriptorSetLayout layout, u32 count, VkDescriptorSet* descriptor_sets) {
    VkDescriptorSetLayout layouts[count];
    for (u32 i = 0; i < count; i++) layouts[i] = layout;

    VkDescriptorSetAllocateInfo info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = device.descriptor_pool,
        .descriptorSetCount = count,
        .pSetLayouts = layouts,
    };

    vkAllocateDescriptorSets(device.logical, &info, descriptor_sets);
}

void freeDescriptorSets(Device device, u32 count, VkDescriptorSet* descriptor_sets) {
    vkFreeDescriptorSets(device.logical, device.descriptor_pool, count, descriptor_sets);
}

void setDescriptorImage(Device device, DeviceLoop* loop, u32 index, VkDescriptorImageInfo image) {
    VkWriteDescriptorSet write = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pImageInfo = &image,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = 1,
        .dstArrayElement = index,
        .dstSet = loop->descriptor_sets[loop->frame],
        .dstBinding = 0,
    };

    vkUpdateDescriptorSets(device.logical, 1, &write, 0, NULL);
    loop->written = true;
}

u32 addDescriptorImage(Device device, DeviceLoop* loop, VkDescriptorImageInfo image) {
    setDescriptorImage(device, loop, loop->set_indices[0], image);
    return loop->set_indices[0]++;
}

u32 addDescriptorTexture(Device device, DeviceLoop* loop, Sampler sampler, Texture texture) {
    VkDescriptorImageInfo info = {
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        .imageView = texture.color_view,
        .sampler = device.samplers[sampler],
    };

    return addDescriptorImage(device, loop, info);
}

void setDescriptorStorageBuffer(Device device, DeviceLoop* loop, u32 index, VkDescriptorBufferInfo buffer) {
    VkWriteDescriptorSet write = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pBufferInfo = &buffer,
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .descriptorCount = 1,
        .dstArrayElement = index,
        .dstSet = loop->descriptor_sets[loop->frame],
        .dstBinding = 1,
    };

    vkUpdateDescriptorSets(device.logical, 1, &write, 0, NULL);
    loop->written = true;
}

u32 addDescriptorStorageBuffer(Device device, DeviceLoop* loop, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range) {
    setDescriptorStorageBuffer(device, loop, loop->set_indices[1], (VkDescriptorBufferInfo){.buffer = buffer, .offset = offset, .range = range});
    return loop->set_indices[1]++;
}

void setDescriptorUniformBuffer(Device device, DeviceLoop* loop, u32 index, VkDescriptorBufferInfo buffer) {
    VkWriteDescriptorSet write = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pBufferInfo = &buffer,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1,
        .dstArrayElement = index,
        .dstSet = loop->descriptor_sets[loop->frame],
        .dstBinding = 2,
    };

    vkUpdateDescriptorSets(device.logical, 1, &write, 0, NULL);
    loop->written = true;
}

u32 addDescriptorUniformBuffer(Device device, DeviceLoop* loop, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range) {
    setDescriptorUniformBuffer(device, loop, loop->set_indices[2], (VkDescriptorBufferInfo){.buffer = buffer, .offset = offset, .range = range});
    return loop->set_indices[2]++;
}

void registerStorageBuffer(Device device, DeviceLoop* loop, VkBuffer buffer, VkDeviceSize size, u32* ids) {
    for (u32 i = 0; i < FRAMES_IN_FLIGHT; i++) ids[i] = addDescriptorStorageBuffer(device, loop, buffer, size * i, size);
}
