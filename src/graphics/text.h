#ifndef TEXT_H
#define TEXT_H

#include <elc/core.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

typedef struct TextRenderer {
    VkPipeline pipeline;
} TextRenderer;

#endif
