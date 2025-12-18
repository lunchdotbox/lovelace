#include "text.h"

#include "buffer.h"
#include "device.h"
#include "device_loop.h"
#include "graphics_pipeline.h"
#include "descriptor.h"
#include "texture.h"
#include "command.h"
#include <vulkan/vulkan_core.h>

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
    setPipelineVertexShader(&config, createShaderModule(device, "spv/text.vert.spv"));
    setPipelineFragmentShader(&config, createShaderModule(device, "spv/text.frag.spv"));
    text_renderer.pipeline = createPipeline(device, config);
    return text_renderer;
}

void destroyTextRenderer(Device device, TextRenderer renderer) {
    vkDestroyPipeline(device.logical, renderer.pipeline, NULL);
}

TextFont createTextFont(Device device, DeviceLoop* loop) {
    TextFont font;
    font.buffer_size = 1;
    font.texture = loadTexture(device, loop, QUEUE_TYPE_GRAPHICS, "images/fonts/minogram_6x10.png");
    font.push.texture_id = addDescriptorTexture(device, loop, 0, font.texture);
    font.buffer = createValidBuffer(device, sizeof(TextCharacter), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, false, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    TextCharacter* character_mapped = mapDeviceMemory(device, font.buffer.memory, 0, sizeof(TextCharacter) * font.buffer_size);
    *character_mapped = (TextCharacter){0};
    character_mapped->transform1[0] = 0.5f / 13.0f;
    character_mapped->transform2[1] = 0.5f / 7.0f;
    character_mapped->transform2[2] = 1.0f;
    character_mapped->character = 13;
    makePermanentBuffers(device, QUEUE_TYPE_GRAPHICS, 1, &font.buffer, (size_t[]){sizeof(TextCharacter) * font.buffer_size}, (VkBufferUsageFlags[]){VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT}, (bool[]){false});
    font.push.transform_buffer_id = addDescriptorStorageBuffer(device, loop, font.buffer.buffer, 0, sizeof(TextCharacter) * font.buffer_size);
    return font;
}

void destroyTextFont(Device device, TextFont font) {
    destroyValidBuffer(device, font.buffer);
    destroyTexture(device, font.texture);
}

void resizeTextFont(Device device, TextFont* font, u64 new_size) {
    ValidBuffer new_buffer = createValidBuffer(device, sizeof(TextCharacter) * new_size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, false, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    VkCommandBuffer command = beginSingleTimeCommands(device, QUEUE_TYPE_GRAPHICS);
    if (new_size > font->buffer_size) vkCmdFillBuffer(command, new_buffer.buffer, font->buffer_size * sizeof(TextCharacter), (new_size - font->buffer_size) * sizeof(TextCharacter), 0);
    commandCopyBuffer(command, font->buffer.buffer, new_buffer.buffer, MIN(font->buffer_size, new_size) * sizeof(TextCharacter));
    destroyValidBuffer(device, font->buffer);
    font->buffer = new_buffer;
    font->buffer_size = new_size;
}

void drawTextFont(VkCommandBuffer command, Device device, TextRenderer renderer, TextFont font) {
    vkCmdBindPipeline(command, VK_PIPELINE_BIND_POINT_GRAPHICS, renderer.pipeline);
    vkCmdPushConstants(command, device.pipeline_layout, VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(TextPushConstant), &font.push);
    vkCmdDraw(command, 6, 1, 0, 0);
}
