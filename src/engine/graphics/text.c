#include "text.h"

#include "buffer.h"
#include "color.h"
#include "device.h"
#include "device_loop.h"
#include "graphics_pipeline.h"
#include "descriptor.h"
#include "texture.h"
#include <cglm/affine2d.h>
#include <cglm/mat3.h>
#include <cglm/vec2.h>
#include <cglm/vec3.h>
#include <fcntl.h>
#include <string.h>
#include <vulkan/vulkan_core.h>

static const u32 atlas_ascii_map[] = {
    ['A'] = 0, ['B'] = 1, ['C'] = 2, ['D'] = 3, ['E'] = 4, ['F'] = 5, ['G'] = 6, ['H'] = 7, ['I'] = 8, ['J'] = 9, ['K'] = 10, ['L'] = 11, ['M'] = 12,
    ['N'] = 13, ['O'] = 14, ['P'] = 15, ['Q'] = 16, ['R'] = 17, ['S'] = 18, ['T'] = 19, ['U'] = 20, ['V'] = 21, ['W'] = 22, ['X'] = 23, ['Y'] = 24, ['Z'] = 25,

    ['a'] = 26, ['b'] = 27, ['c'] = 28, ['d'] = 29, ['e'] = 30, ['f'] = 31, ['g'] = 32, ['h'] = 33, ['i'] = 34, ['j'] = 35, ['k'] = 36, ['l'] = 37, ['m'] = 38,
    ['n'] = 39, ['o'] = 40, ['p'] = 41, ['q'] = 42, ['r'] = 43, ['s'] = 44, ['t'] = 45, ['u'] = 46, ['v'] = 47, ['w'] = 48, ['x'] = 49, ['y'] = 50, ['z'] = 51,

    ['0'] = 52, ['1'] = 53, ['2'] = 54, ['3'] = 55, ['4'] = 56, ['5'] = 57, ['6'] = 58, ['7'] = 59, ['8'] = 60, ['9'] = 61,

    ['+'] = 62, ['-'] = 63, ['='] = 64, ['('] = 65, [')'] = 66, ['['] = 67, [']'] = 68, ['{'] = 69, ['}'] = 70, ['<'] = 71, ['>'] = 72, ['/'] = 73, ['*'] = 74,
    [':'] = 75, ['#'] = 76, ['%'] = 77, ['!'] = 78, ['?'] = 79, ['.'] = 80, [','] = 81, ['\''] = 82, ['"'] = 83, ['@'] = 84, ['&'] = 85, ['$'] = 86,
};

TextRenderer createTextRenderer(Device device, PipelineConfig config) {
    TextRenderer text_renderer;
    config.attributes = NULL;
    config.n_attributes = 0;
    config.bindings = NULL;
    config.n_bindings = 0;
    config.color_attachments->blendEnable = VK_TRUE;
    config.color_attachments->srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    config.color_attachments->dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    config.color_attachments->colorBlendOp = VK_BLEND_OP_ADD;
    config.enable_depth = VK_FALSE;
    config.cull_mode = VK_CULL_MODE_NONE;
    setPipelineVertexShader(&config, createShaderModule(device, "spv/text.vert.spv"));
    setPipelineFragmentShader(&config, createShaderModule(device, "spv/text.frag.spv"));
    text_renderer.pipeline = createPipeline(device, config);
    return text_renderer;
}

void destroyTextRenderer(Device device, TextRenderer renderer) {
    vkDestroyPipeline(device.logical, renderer.pipeline, NULL);
}

TextFont createTextFont(Device device, DeviceLoop* loop, u64 buffer_size, const char* atlas_path) {
    TextFont font = {0};
    font.buffer_size = buffer_size;
    font.texture = loadTexture(device, loop, QUEUE_TYPE_GRAPHICS, atlas_path);
    font.texture_id = addDescriptorTexture(device, loop, SAMPLER_NEAREST, font.texture);
    font.buffer = createValidBuffer(device, sizeof(TextCharacter) * font.buffer_size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT, false, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    font.buffer_id = addDescriptorStorageBuffer(device, loop, font.buffer.buffer, 0, sizeof(TextCharacter) * font.buffer_size);
    font.buffer_mapped = mapDeviceMemory(device, font.buffer.memory, 0, font.buffer_size);
    return font;
}

void destroyTextFont(Device device, TextFont font) {
    vkUnmapMemory(device.logical, font.buffer.memory);
    destroyValidBuffer(device, font.buffer);
    destroyTexture(device, font.texture);
}

void drawTextFont(VkCommandBuffer command, Device device, TextRenderer renderer, TextFont* font, float aspect) {
    TextPushConstant push = {.texture_id = font->texture_id, .buffer_id = font->buffer_id, .aspect = aspect};
    flushDeviceMemory(device, font->buffer.memory, 0, VK_WHOLE_SIZE);
    vkCmdBindPipeline(command, VK_PIPELINE_BIND_POINT_GRAPHICS, renderer.pipeline);
    vkCmdPushConstants(command, device.pipeline_layout, VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(TextPushConstant), &push);
    vkCmdDraw(command, 6, font->n_text, 0, 0);
    font->n_text = 0;
}

void addFontCharacters(TextFont* font, TextCharacter* text, u64 n_text) {
    memcpy(&font->buffer_mapped[font->n_text], text, sizeof(TextCharacter) * n_text);
    font->n_text += n_text;
}

TextCharacter makeTextCharacter(mat3 transform, u32 text, Color color) {
    TextCharacter character = {0};
    glm_vec3_copy(transform[0], character.transform1);
    glm_vec3_copy(transform[1], character.transform2);
    glm_vec3_copy(transform[2], character.transform3);
    character.character = text;
    character.color = color;
    return character;
}

void addFontLetter(TextFont* font, mat3 transform, Color color, char letter) {
    font->buffer_mapped[font->n_text++] = makeTextCharacter(transform, atlas_ascii_map[letter], color);
}

void addFontText(TextFont* font, mat3 trans, Color color, const char* text) {
    vec2 offset = {0};
    for (const char* letter = text; *letter != '\0'; letter++) {
        if (*letter == '\n') {
            offset[0] = 0.0f;
            offset[1] += (13.0f / 7.0f) / 2.0f;
        } else if (*letter == ' ') offset[0] += (7.0f / 13.0f) * 0.75;
        else {
            mat3 transform;
            glm_translate2d_make(transform, offset);
            glm_mat3_mul(trans, transform, transform);
            addFontLetter(font, transform, color, *letter);
            offset[0] += 7.0f / 13.0f;
        }
    }
}

void addTextPositioned(TextFont* font, vec2 position, vec2 scale, Color color, const char* text) {
    mat3 transform;
    glm_translate2d_make(transform, position);
    glm_scale2d(transform, scale);
    addFontText(font, transform, color, text);
}

void addFontIcon(TextFont *font, mat3 trans, Color color, TextIcon icon) {
    mat3 transform;
    switch (icon) {
        case TEXT_ICON_FULL:
            glm_scale2d_make(transform, (vec2){13.0f / 7.0f, 1.0f});
            glm_mat3_mul(trans, transform, transform);
            font->buffer_mapped[font->n_text++] = makeTextCharacter(transform, 87, color);
            break;
        case TEXT_ICON_BOX:
            font->buffer_mapped[font->n_text++] = makeTextCharacter(trans, 88, color);
            break;
        case TEXT_ICON_CHECK:
            font->buffer_mapped[font->n_text++] = makeTextCharacter(trans, 89, color);
            break;
        case TEXT_ICON_CROSS:
            addFontLetter(font, trans, color, 'x');
            break;
    }
}

void textDimensions(const char* text, vec2 dimensions) {
    vec2 position = {0.0f, 7.0f / 13.0f};
    glm_vec2_copy(position, dimensions);
    for (const char* letter = text; *letter != '\0'; letter++) {
        if (*letter == '\n') {
            position[0] = 0.0f;
            position[1] += 7.0f / 13.0f;
        } else if (*letter == ' ') position[0] += (7.0f / 13.0f) / 2.0f;
        else position[0] += 7.0f / 13.0f;
        if (position[0] > dimensions[0]) dimensions[0] = position[0];
        if (position[1] > dimensions[1]) dimensions[1] = position[1];
    }
    dimensions[1] *= 13.0f / 7.0f;
}

void addTextRectangle(TextFont* font, Color color, vec2 position, vec2 size) {
    mat3 transform;
    glm_translate2d_make(transform, position);
    glm_scale2d(transform, size);
    addFontIcon(font, transform, color, TEXT_ICON_FULL);
}

void addTextCentered(TextFont* font, vec2 position, vec2 scale, Color color, const char* text) {
    vec2 dimensions, pos;
    textDimensions(text, dimensions);
    glm_vec2_mul(dimensions, scale, dimensions);
    glm_vec2_scale(dimensions, 0.5f, dimensions);
    glm_vec2_sub(position, dimensions, pos);
    addTextPositioned(font, pos, scale, color, text);
}
