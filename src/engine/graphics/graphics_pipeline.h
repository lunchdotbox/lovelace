#ifndef ENGINE_GRAPHICS_PIPELINE_H
#define ENGINE_GRAPHICS_PIPELINE_H

#include "device.h"
#include "window.h"
#include <vulkan/vulkan_core.h>

typedef struct PipelineConfig {
    VkRenderPass render_pass;
    VkShaderModule fragment, vertex, control, evaluation;
    VkPipelineColorBlendAttachmentState* color_attachments;
    VkVertexInputAttributeDescription* attributes;
    VkVertexInputBindingDescription* bindings;
    VkPolygonMode polygon_mode;
    VkCullModeFlags cull_mode;
    VkBool32 enable_depth;
    VkCompareOp depth_op;
    VkExtent2D extent;
    VkPrimitiveTopology primitive_topology;
    u32 n_bindings, n_attributes, patch_points, n_color_attachments;
} PipelineConfig;

VkPipelineLayout createPipelineLayout(Device device, u64 push_size, VkShaderStageFlags push_stage);
VkShaderModule createShaderModule(Device device, const char* file_path);
void destroyPipelineConfig(PipelineConfig config, Device device);
VkPipeline createPipeline(Device device, PipelineConfig config);
void setPipelineVertexShader(PipelineConfig* config, VkShaderModule shader);
void setPipelineFragmentShader(PipelineConfig* config, VkShaderModule shader);
void setPipelineExtent(PipelineConfig* config, VkExtent2D extent);
void setPipelineRenderPass(PipelineConfig* config, VkRenderPass render_pass);
void setPipelineWindow(PipelineConfig* config, Window window);

#endif
