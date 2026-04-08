#include "simple_draw.h"

#include "command.h"
#include "device_loop.h"
#include "graphics_pipeline.h"
#include "uniform.h"

DiffuseRenderer createDiffuseRenderer(Device device, DeviceLoop* loop, PipelineConfig config) {
    DiffuseRenderer renderer;
    renderer.uniform = createUniformBuffer(device, sizeof(mat4));
    registerUniformBuffer(device, loop, renderer.uniform, sizeof(mat4), renderer.uniform_ids);
    setPipelineVertexShader(&config, createShaderModule(device, "spv/terrain.vert.spv"));
    setPipelineFragmentShader(&config, createShaderModule(device, "spv/diffuse.frag.spv"));
    renderer.pipeline = createPipeline(device, config);
    return renderer;
}

void destroyDiffuseRenderer(Device device, DiffuseRenderer renderer) {
    vkDestroyPipeline(device.logical, renderer.pipeline, NULL);
    destroyUniformBuffer(device, renderer.uniform);
}

void recreateDiffuseRenderer(Device device, DiffuseRenderer* renderer, PipelineConfig config) {
    vkDestroyPipeline(device.logical, renderer->pipeline, NULL);
    setPipelineVertexShader(&config, createShaderModule(device, "spv/terrain.vert.spv"));
    setPipelineFragmentShader(&config, createShaderModule(device, "spv/diffuse.frag.spv"));
    renderer->pipeline = createPipeline(device, config);
}

void setRendererCamera(Device device, DeviceLoop loop, DiffuseRenderer renderer, Camera camera) {
    mat4 view_matrix;
    getCameraMatrix(camera, view_matrix);
    updateUniformBuffer(device, loop, renderer.uniform, view_matrix, sizeof(mat4));
}

void drawTexturedModel(DeviceLoop loop, DiffuseRenderer renderer, Device device, Model model, u32 texture_id, mat4 transform) {
    CameraPushConstant push = {.texture_id = texture_id, .uniform_id = renderer.uniform_ids[loop.frame]};
    glm_mat4_copy(transform, push.model_matrix);
    commandPushConstants(loop.commands[loop.frame], device, VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT, sizeof(push), &push);
    drawModel(loop.commands[loop.frame], model);
}
