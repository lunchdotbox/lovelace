#ifndef MODEL_H
#define MODEL_H

#include <elc/core.h>
#include "device.h"
#include "graphics_pipeline.h"

#define MODEL_PIPELINE_CONFIG ((PipelineConfig){\
    .primitive_topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,\
    .cull_mode = VK_CULL_MODE_NONE,\
    .enable_depth = VK_TRUE,\
    .depth_op = VK_COMPARE_OP_LESS,\
    .bindings = &(VkVertexInputBindingDescription){.binding = 0, .stride = sizeof(VulkanVertex), .inputRate = VK_VERTEX_INPUT_RATE_VERTEX},\
    .n_bindings = 1,\
    .color_attachments = &(VkPipelineColorBlendAttachmentState){\
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,\
        .blendEnable = VK_FALSE,\
    },\
    .n_color_attachments = 1,\
    .attributes = (VkVertexInputAttributeDescription[]){\
        {.binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .location = 0, .offset = offsetof(VulkanVertex, position)},\
        {.binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .location = 1, .offset = offsetof(VulkanVertex, normal)},\
        {.binding = 0, .format = VK_FORMAT_R32G32_SFLOAT, .location = 2, .offset = offsetof(VulkanVertex, uv)},\
    },\
    .n_attributes = 3,\
})

typedef struct VulkanVertex {
    vec3 position, normal;
    vec2 uv;
} VulkanVertex;

typedef struct Model {
    VkBuffer vertex_buffer, index_buffer;
    VkDeviceMemory vertex_memory, index_memory;
    u32 vertex_count, index_count;
} Model;

Model createModel(Device device, u32 vertex_size, u32 vertex_count, u32 index_count, void** vertex_mapped, void** index_mapped);
void destroyModel(Device device, Model model);
void finishModelUpload(Device device, Model* model, u32 vertex_size);
PipelineConfig windowPipelineConfig(Window window);

#endif
