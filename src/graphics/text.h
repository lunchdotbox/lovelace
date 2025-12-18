#ifndef TEXT_H
#define TEXT_H

#include "buffer.h"
#include "texture.h"
#include <elc/core.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "graphics_pipeline.h"

typedef struct TextPushConstant {
    u32 texture_id;
    u32 transform_buffer_id;
} TextPushConstant;

typedef struct TextCharacter {
    vec3 transform1;
    u32 character;
    vec3 transform2;
    float padding1;
    vec3 transform3;
    float padding2;
} TextCharacter; // TODO: optimize for better memory usage

typedef struct TextRenderer {
    VkPipeline pipeline;
} TextRenderer;

typedef struct TextFont {
    Texture texture;
    ValidBuffer buffer;
    u64 buffer_size;
    TextPushConstant push;
} TextFont;

TextRenderer createTextRenderer(Device device, PipelineConfig config);
void destroyTextRenderer(Device device, TextRenderer renderer);
TextFont createTextFont(Device device, DeviceLoop* loop);
void destroyTextFont(Device device, TextFont font);
void resizeTextFont(Device device, TextFont* font, u64 new_size);
void drawTextFont(VkCommandBuffer command, Device device, TextRenderer renderer, TextFont font);

#endif
