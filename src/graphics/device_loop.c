#include "device_loop.h"

#include "device.h"
#include "descriptor.h"
#include "command.h"
#include "synchronization.h"
#include <elc/core.h>
#include <vulkan/vulkan_core.h>

DeviceLoop createDeviceLoop(Device device, QueueType type) {
    DeviceLoop loop = {0};
    allocateDescriptorSets(device, device.set_layout, FRAMES_IN_FLIGHT, loop.descriptor_sets);
    allocateCommandBuffers(device, type, FRAMES_IN_FLIGHT, loop.commands);
    for (u32 i = 0; i < FRAMES_IN_FLIGHT; i++) {
        loop.semaphores[i] = createSemaphore(device);
        loop.fences[i] = createFence(device);
    }
    return loop;
}

void destroyDeviceLoop(DeviceLoop loop, Device device, QueueType type) {
    for (u32 i = 0; i < FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(device.logical, loop.semaphores[i], NULL);
        vkDestroyFence(device.logical, loop.fences[i], NULL);
    }
    freeCommandBuffers(device, type, FRAMES_IN_FLIGHT, loop.commands);
    freeDescriptorSets(device, FRAMES_IN_FLIGHT, loop.descriptor_sets);
}

VkDescriptorSet getLoopSet(DeviceLoop loop) {
    return loop.descriptor_sets[loop.frame];
}

void propogateDescriptorWrites(Device device, DeviceLoop* loop) {
    if (loop->written) {
        VkCopyDescriptorSet copies[2];
        for (u32 i = 0; i < ARRAY_LENGTH(copies); i++) // TODO: make a seperate written flag for each binding
            copies[i] = (VkCopyDescriptorSet){
                .sType = VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET,
                .srcSet = getLoopSet(*loop),
                .dstSet = loop->descriptor_sets[(loop->frame + 1) % FRAMES_IN_FLIGHT],
                .descriptorCount = 10000,
                .srcBinding = i,
                .dstBinding = i,
            };

        vkUpdateDescriptorSets(device.logical, 0, NULL, ARRAY_LENGTH(copies), copies);
    }
    loop->written = false;
}

void beginDeviceLoop(Device device, DeviceLoop* loop) {
    vkWaitForFences(device.logical, 1, &loop->fences[loop->frame], VK_TRUE, UINT64_MAX);
    propogateDescriptorWrites(device, loop);
    vkResetFences(device.logical, 1, &loop->fences[loop->frame]);
}

VkSemaphore getLoopSemaphore(DeviceLoop loop) {
    return loop.semaphores[loop.frame];
}

VkFence getLoopFence(DeviceLoop loop) {
    return loop.fences[loop.frame];
}

void submitCommandBuffer(Device device, DeviceLoop* loop, VkCommandBuffer command, VkSemaphore signal) {
    VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &(VkSemaphore){getLoopSemaphore(*loop)},
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &signal,
        .commandBufferCount = 1,
        .pCommandBuffers = &command,
        .pWaitDstStageMask = (VkShaderStageFlags[]){VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT},
    };

    vkQueueSubmit(device.graphics_queue, 1, &submit_info, getLoopFence(*loop));
    loop->frame = (loop->frame + 1) % FRAMES_IN_FLIGHT;
}
