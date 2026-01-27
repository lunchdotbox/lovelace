#ifndef ENGINE_GRAPHICS_BLOOM_H
#define ENGINE_GRAPHICS_BLOOM_H

#include "device_loop.h"

typedef struct BloomEffect {
    VkRenderPass render_pass;
    VkFramebuffer framebuffers[FRAMES_IN_FLIGHT];
    VkImage color_images[FRAMES_IN_FLIGHT];
} BloomEffect;

#endif
