#ifndef DESCRIPTOR_H
#define DESCRIPTOR_H

#include "device.h"
#include "device_loop.h"
#include "texture.h"

VkDescriptorPool createDescriptorPool(Device device, u32 set_count, u32 descriptor_count);
VkDescriptorSetLayout createSetLayout(Device device, u32 descriptor_count);
void allocateDescriptorSets(Device device, VkDescriptorSetLayout layout, u32 count, VkDescriptorSet* descriptor_sets);
void freeDescriptorSets(Device device, u32 count, VkDescriptorSet* descriptor_sets);
void setDescriptorImage(Device device, DeviceLoop* loop, u32 index, VkDescriptorImageInfo image);
u32 addDescriptorImage(Device device, DeviceLoop* loop, VkDescriptorImageInfo image);
u32 addDescriptorTexture(Device device, DeviceLoop* loop, u32 sampler, Texture texture);
void setDescriptorStorageBuffer(Device device, DeviceLoop* loop, u32 index, VkDescriptorBufferInfo buffer);
u32 addDescriptorStorageBuffer(Device device, DeviceLoop* loop, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range);

#endif
