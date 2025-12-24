#ifndef SIMPLE_DRAW_H
#define SIMPLE_DRAW_H

#include "camera.h"
#include "command.h"
#include "descriptor.h"
#include "device.h"
#include "device_loop.h"
#include "graphics_pipeline.h"
#include "model.h"
#include "texture.h"
#include "uniform.h"
#include <cglm/mat4.h>
#include <vulkan/vulkan_core.h>

typedef struct DiffuseRenderer {
    UniformBuffer uniform;
    VkPipeline pipeline;
    u32 uniform_id;
} DiffuseRenderer;

DiffuseRenderer createDiffuseRenderer(Device device, DeviceLoop* loop, PipelineConfig config, u32 max_textures, u32 max_models);
void destroyDiffuseRenderer(Device device, DiffuseRenderer renderer);
void setRendererCamera(Device device, DiffuseRenderer renderer, Camera camera);
void drawTexturedModel(VkCommandBuffer command, DiffuseRenderer renderer, Device device, Model model, u32 texture_id, mat4 transform);

#endif
