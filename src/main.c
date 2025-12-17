#include "graphics/buffer.h"
#include "graphics/descriptor.h"
#include <cglm/affine2d.h>
#include <vulkan/vulkan_core.h>
#define GLM_FORCE_DEPTH_ZERO_TO_ONE 1
#define GLM_FORCE_RADIANS 1

#include "graphics/instance.h"
#include "graphics/device.h"
#include "graphics/window.h"
#include "graphics/graphics_pipeline.h"
#include "graphics/model.h"
#include "graphics/texture.h"

typedef struct TextPushConstant {
    u32 texture_id;
    u32 transform_buffer_id;
} TextPushConstant;

typedef struct TextTransform {
    vec3 transform1;
    u32 character;
    vec3 transform2;
    float padding1;
    vec3 transform3;
    float padding2;
} TextTransform;

TextTransform makeTextScale(vec2 scale) {
    TextTransform transform;
    glm_vec3_zero(transform.transform1);
    transform.transform1[0] = scale[0];
    glm_vec3_zero(transform.transform2);
    transform.transform2[1] = scale[1];
    glm_vec3_zero(transform.transform3);
    transform.transform3[2] = 1.0f;
    return transform;
}

int main() {
    glfwInit();
    VkInstance instance = createInstance();
    Device device = createDevice(instance);
    Window window = createWindow(device, instance, 800, 600, "vulkan renderer");

    PipelineConfig pipeline_config;
    MODEL_PIPELINE_CONFIG(pipeline_config);
    pipeline_config.attributes = NULL;
    pipeline_config.n_attributes = 0;
    pipeline_config.bindings = NULL;
    pipeline_config.n_bindings = 0;
    pipeline_config.color_attachments->blendEnable = VK_TRUE;
    pipeline_config.color_attachments->srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    pipeline_config.color_attachments->dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    pipeline_config.color_attachments->colorBlendOp = VK_BLEND_OP_ADD;
    setPipelineWindow(&pipeline_config, window);
    setPipelineVertexShader(&pipeline_config, createShaderModule(device, "spv/text.vert.spv"));
    setPipelineFragmentShader(&pipeline_config, createShaderModule(device, "spv/text.frag.spv"));
    VkPipeline pipeline = createPipeline(device, pipeline_config);

    TextPushConstant push;
    Texture font_atlas = loadTexture(device, &window.device_loop, QUEUE_TYPE_GRAPHICS, "images/fonts/minogram_6x10.png");
    push.texture_id = addDescriptorTexture(device, &window.device_loop, 0, font_atlas);
    ValidBuffer transform_buffer = createValidBuffer(device, sizeof(TextTransform), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, false, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    TextTransform* transform_mapped = mapDeviceMemory(device, transform_buffer.memory, 0, sizeof(TextTransform));
    *transform_mapped = makeTextScale((vec2){0.5f / 13, 0.5f / 7});
    transform_mapped->character = 13;
    makePermanentBuffers(device, QUEUE_TYPE_GRAPHICS, 1, &transform_buffer, (size_t[]){sizeof(TextTransform)}, (VkBufferUsageFlags[]){VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT}, (bool[]){false});
    push.transform_buffer_id = addDescriptorStorageBuffer(device, &window.device_loop, transform_buffer.buffer, 0, sizeof(TextTransform));

    while (!glfwWindowShouldClose(window.window)) {
        glfwPollEvents();

        u32 image = beginWindowFrame(&window, device, (VkClearColorValue){0, 20, 255, 0});

        vkCmdBindPipeline(currentCommand(window), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
        vkCmdPushConstants(currentCommand(window), device.pipeline_layout, VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(TextPushConstant), &push);
        vkCmdDraw(currentCommand(window), 6, 1, 0, 0);

        endWindowFrame(&window, device, image);
    }

    vkDeviceWaitIdle(device.logical);

    destroyValidBuffer(device, transform_buffer);
    destroyTexture(device, font_atlas);
    vkDestroyPipeline(device.logical, pipeline, NULL);
    destroyWindow(window, device, instance);
    destroyDevice(device, instance);
    vkDestroyInstance(instance, NULL);
    glfwTerminate();
    return 0;
}
