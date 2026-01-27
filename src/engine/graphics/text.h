#ifndef ENGINE_GRAPHICS_TEXT_H
#define ENGINE_GRAPHICS_TEXT_H

#include "buffer.h"
#include "device_loop.h"
#include "color.h"
#include "texture.h"
#include <cglm/vec2.h>
#include <elc/core.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "graphics_pipeline.h"

typedef enum TextIcon {TEXT_ICON_FULL = 0, TEXT_ICON_BOX, TEXT_ICON_CHECK, TEXT_ICON_CROSS} TextIcon;

typedef struct TextPushConstant {
    u32 texture_id;
    u32 buffer_id;
    float aspect;
} TextPushConstant;

typedef struct TextCharacter {
    vec3 transform1;
    u32 character;
    vec3 transform2;
    Color color;
    vec3 transform3;
    float padding2;
} TextCharacter; // TODO: optimize for better memory usage

typedef struct TextRenderer {
    VkPipeline pipeline;
} TextRenderer;

typedef struct TextFont {
    Texture texture;
    ValidBuffer buffer; // TODO: figure out if this should have two buffers instead
    TextCharacter* buffer_mapped;
    u64 buffer_size, n_text;
    u32 texture_id, buffer_id;
} TextFont;

TextRenderer createTextRenderer(Device device, PipelineConfig config);
void destroyTextRenderer(Device device, TextRenderer renderer);
TextFont createTextFont(Device device, DeviceLoop* loop, u64 buffer_size, const char* atlas_path);
void destroyTextFont(Device device, TextFont font);
void drawTextFont(VkCommandBuffer command, Device device, TextRenderer renderer, TextFont* font, float aspect);
void addFontCharacters(TextFont* font, TextCharacter* text, u64 n_text);
TextCharacter makeTextCharacter(mat3 transform, u32 text, Color color);
void addFontLetter(TextFont* font, mat3 transform, Color color, char letter);
void addFontText(TextFont* font, mat3 trans, Color color, const char* text);
void addTextPositioned(TextFont* font, vec2 position, vec2 scale, Color color, const char* text);
void addFontIcon(TextFont* font, mat3 trans, Color color, TextIcon icon);
void textDimensions(const char* text, vec2 dimensions);
void addTextRectangle(TextFont* font, Color color, vec2 position, vec2 size);
void addTextCentered(TextFont* font, vec2 position, vec2 scale, Color color, const char* text);

#endif
