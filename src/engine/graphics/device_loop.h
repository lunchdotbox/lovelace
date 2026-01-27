#ifndef ENGINE_GRAPHICS_DEVICE_LOOP_H
#define ENGINE_GRAPHICS_DEVICE_LOOP_H

#include "device.h"

#define FRAMES_IN_FLIGHT 2

typedef struct DeviceLoop {
    VkDescriptorSet descriptor_sets[FRAMES_IN_FLIGHT];
    VkCommandBuffer commands[FRAMES_IN_FLIGHT];
    VkSemaphore semaphores[FRAMES_IN_FLIGHT];
    VkFence fences[FRAMES_IN_FLIGHT];
    u32 frame, set_indices[3]; // TODO: make thise more clean maybe
    bool written;
} DeviceLoop;

DeviceLoop createDeviceLoop(Device device, QueueType type);
void destroyDeviceLoop(DeviceLoop loop, Device device, QueueType type);
VkDescriptorSet getLoopSet(DeviceLoop loop);
void propogateDescriptorWrites(Device device, DeviceLoop* loop);
void beginDeviceLoop(Device device, DeviceLoop* loop);
VkSemaphore getLoopSemaphore(DeviceLoop loop);
VkFence getLoopFence(DeviceLoop loop);
void submitCommandBuffer(Device device, DeviceLoop* loop, VkCommandBuffer command, VkSemaphore signal);

#endif
