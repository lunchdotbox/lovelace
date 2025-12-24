#include "simple_draw.h"

DiffuseRenderer createDiffuseRenderer(Device device, DeviceLoop* loop, PipelineConfig config, u32 max_textures, u32 max_models) {
    DiffuseRenderer renderer;
    renderer.uniform = createUniformBuffer(device, sizeof(mat4));
    renderer.uniform_id = addDescriptorUniformBuffer(device, loop, renderer.uniform.buffer.buffer, 0, VK_WHOLE_SIZE);
    config.polygon_mode = VK_POLYGON_MODE_LINE;
    setPipelineVertexShader(&config, createShaderModule(device, "spv/terrain.vert.spv"));
    setPipelineFragmentShader(&config, createShaderModule(device, "spv/diffuse.frag.spv"));
    renderer.pipeline = createPipeline(device, config);
    return renderer;
}

void destroyDiffuseRenderer(Device device, DiffuseRenderer renderer) {
    vkDestroyPipeline(device.logical, renderer.pipeline, NULL);
    destroyUniformBuffer(device, renderer.uniform);
}

void setRendererCamera(Device device, DiffuseRenderer renderer, Camera camera) {
    mat4 view_matrix;
    getCameraMatrix(camera, view_matrix);
    updateUniformBuffer(device, renderer.uniform, view_matrix, sizeof(mat4));
}

void drawTexturedModel(VkCommandBuffer command, DiffuseRenderer renderer, Device device, Model model, u32 texture_id, mat4 transform) {
    CameraPushConstant push = {.texture_id = texture_id, .uniform_id = renderer.uniform_id};
    glm_mat4_copy(transform, push.model_matrix);
    commandPushConstants(command, device, VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT, sizeof(push), &push);
    drawModel(command, model);
}
