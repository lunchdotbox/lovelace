#ifndef ENGINE_GRAPHICS_SIMPLE_DRAW_H
#define ENGINE_GRAPHICS_SIMPLE_DRAW_H

#include "buffer.h"
#include "camera.h"
#include "device.h"
#include "device_loop.h"
#include "graphics_pipeline.h"
#include "model.h"
#include "uniform.h"

#include <cglm/mat4.h>

typedef struct DiffuseRenderer {
    UniformBuffer uniform; // TODO: make it use a buffer per frame in flight
    VkPipeline pipeline;
    u32 uniform_id;
} DiffuseRenderer;

typedef struct RenderedComponent {
    Model* model;
    u32 texture_id;
} RenderedComponent;

DiffuseRenderer createDiffuseRenderer(Device device, DeviceLoop* loop, PipelineConfig config);
void destroyDiffuseRenderer(Device device, DiffuseRenderer renderer);
void recreateDiffuseRenderer(Device device, DiffuseRenderer* renderer, PipelineConfig config);
void setRendererCamera(Device device, DiffuseRenderer renderer, Camera camera);
void drawTexturedModel(VkCommandBuffer command, DiffuseRenderer renderer, Device device, Model model, u32 texture_id, mat4 transform);

#endif
