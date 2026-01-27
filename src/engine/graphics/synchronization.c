#include "synchronization.h"

#include "device.h"

VkSemaphore createSemaphore(Device device) {
    VkSemaphoreCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    };

    VkSemaphore semaphore;
    vkCreateSemaphore(device.logical, &info, NULL, &semaphore);

    return semaphore;
}

VkFence createFence(Device device) {
    VkFenceCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT,
    };

    VkFence fence;
    vkCreateFence(device.logical, &info, NULL, &fence);

    return fence;
}
