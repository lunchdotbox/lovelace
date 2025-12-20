#include "graphics/buffer.h"
#include "graphics/color.h"
#include "graphics/command.h"
#include "graphics/descriptor.h"
#include "graphics/wavefront.h"
#include <GLFW/glfw3.h>
#include <cglm/affine2d.h>
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
#include "graphics/text.h"
#include "graphics/render_target.h"
#include "graphics/camera.h"

int main() {
    glfwInit();
    VkInstance instance = createInstance();
    Device device = createDevice(instance);
    Window window = createWindow(device, instance, 800, 600, "vulkan renderer");

    // TextRenderer text_renderer = createTextRenderer(device, windowPipelineConfig(window));
    // TextFont text_font = createTextFont(device, &window.device_loop, ELC_KILOBYTE, "images/fonts/minogram_6x10.png");

    PipelineConfig pipeline_config = windowPipelineConfig(window);
    setPipelineVertexShader(&pipeline_config, createShaderModule(device, "spv/terrain.vert.spv"));
    setPipelineFragmentShader(&pipeline_config, createShaderModule(device, "spv/diffuse.frag.spv"));
    VkPipeline pipeline = createPipeline(device, pipeline_config);

    Camera camera = createCamera();

    Model model = loadWavefront(device, "models/cat.obj");
    Texture texture = loadTexture(device, &window.device_loop, QUEUE_TYPE_GRAPHICS, "images/cat.png");
    addDescriptorTexture(device, &window.device_loop, SAMPLER_LINEAR, texture);

    while (!glfwWindowShouldClose(window.window)) {
        glfwPollEvents();

        updateCamera(&camera, window);

        u32 image = beginWindowFrame(&window, device);
        beginWindowPass(window, image, (vec4){0.25f, 0.25f, 0.25f, 0.0f});

        CameraPushConstant push = getCameraPush(camera);
        commandPushConstants(currentCommand(window), device, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(push), &push);

        vkCmdBindPipeline(currentCommand(window), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

        drawModel(currentCommand(window), model);

        endWindowFrame(&window, device, image);
    }

    vkDeviceWaitIdle(device.logical);

    destroyTexture(device, texture);
    destroyModel(device, model);
    vkDestroyPipeline(device.logical, pipeline, NULL);
    // destroyTextFont(device, text_font);
    // destroyTextRenderer(device, text_renderer);
    destroyWindow(window, device, instance);
    destroyDevice(device, instance);
    vkDestroyInstance(instance, NULL);
    glfwTerminate();
    return 0;
}
