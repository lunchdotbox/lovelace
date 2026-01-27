#ifndef ENGINE_GRAPHICS_SYNCHRONIZATION_H
#define ENGINE_GRAPHICS_SYNCHRONIZATION_H

#include "device.h"

VkSemaphore createSemaphore(Device device);
VkFence createFence(Device device);

#endif
