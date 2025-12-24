#include "graphics/command.h"
#include "graphics/descriptor.h"
#include "formats/wavefront.h"
#include "graphics/uniform.h"
#include "physics/mesh_inertia.h"
#include <GLFW/glfw3.h>
#include <cglm/affine-pre.h>
#include <cglm/affine2d.h>
#include <cglm/io.h>
#include <cglm/mat4.h>
#include <cglm/types.h>
#include <elc/core.h>
#include <vulkan/vulkan_core.h>
#define GLM_FORCE_DEPTH_ZERO_TO_ONE 1
#define GLM_FORCE_RADIANS 1
#include "graphics/instance.h"
#include "graphics/device.h"
#include "graphics/window.h"
#include "graphics/graphics_pipeline.h"
#include "graphics/model.h"
#include "graphics/texture.h"
#include "graphics/camera.h"
#include "graphics/text.h"

int main() {
    glfwInit();
    VkInstance instance = createInstance();
    Device device = createDevice(instance);
    Window window = createWindow(device, instance, 800, 600, "vulkan renderer");

    TextRenderer text_renderer = createTextRenderer(device, windowPipelineConfig(window));
    TextFont text_font = createTextFont(device, &window.device_loop, ELC_KILOBYTE, "images/fonts/minogram_6x10.png");

    PipelineConfig pipeline_config = windowPipelineConfig(window);
    pipeline_config.polygon_mode = VK_POLYGON_MODE_LINE;
    setPipelineVertexShader(&pipeline_config, createShaderModule(device, "spv/terrain.vert.spv"));
    setPipelineFragmentShader(&pipeline_config, createShaderModule(device, "spv/diffuse.frag.spv"));
    VkPipeline pipeline = createPipeline(device, pipeline_config);

    Camera camera = createCamera();
    glfwSetInputMode(window.window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    HostMesh mesh = loadWavefront("models/piston_rod.obj");
    mat3 inertia;
    vec3 com;
    float mass;
    computeMeshInertia(mesh, 1.0f, inertia, com, &mass);
    glm_vec3_print(com, stdout);

    Model model = hostMeshToModel(device, mesh);
    destroyHostMesh(mesh);
    Texture texture = loadTexture(device, &window.device_loop, QUEUE_TYPE_GRAPHICS, "images/cat.png");
    CameraPushConstant push;
    glm_mat4_identity(push.model_matrix);
    push.texture_id = addDescriptorTexture(device, &window.device_loop, SAMPLER_LINEAR, texture);
    UniformBuffer uniform = createUniformBuffer(device, sizeof(mat4));
    push.uniform_id = addDescriptorUniformBuffer(device, &window.device_loop, uniform.buffer.buffer, 0, VK_WHOLE_SIZE);

    while (!glfwWindowShouldClose(window.window)) {
        glfwPollEvents();

        updateCamera(&camera, window);

        u32 image = beginWindowFrame(&window, device);
        beginWindowPass(window, image, (vec4){0.25f, 0.25f, 0.25f, 0.0f});

        mat4 view_matrix;
        getCameraMatrix(camera, view_matrix);
        updateUniformBuffer(device, uniform, view_matrix, sizeof(view_matrix));

        glm_mat4_identity(push.model_matrix);
        commandPushConstants(currentCommand(window), device, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(push), &push);

        vkCmdBindPipeline(currentCommand(window), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

        drawModel(currentCommand(window), model);

        glm_translate(push.model_matrix, com);
        glm_scale(push.model_matrix, (vec3){0.1f, 0.1f, 0.1f});
        commandPushConstants(currentCommand(window), device, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(push), &push);

        drawModel(currentCommand(window), model);

        addTextPositioned(&text_font, (vec2){0}, (vec2){0.1f, 0.1f}, COLOR_RED, "deez");

        drawTextFont(currentCommand(window), device, text_renderer, &text_font, windowAspect(window));

        endWindowFrame(&window, device, image);
    }

    vkDeviceWaitIdle(device.logical);

    destroyUniformBuffer(device, uniform);
    destroyTexture(device, texture);
    destroyModel(device, model);
    vkDestroyPipeline(device.logical, pipeline, NULL);
    destroyTextFont(device, text_font);
    destroyTextRenderer(device, text_renderer);
    destroyWindow(window, device, instance);
    destroyDevice(device, instance);
    vkDestroyInstance(instance, NULL);
    glfwTerminate();
    return 0;
}
