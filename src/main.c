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
#include "graphics/text.h"

int main() {
    glfwInit();
    VkInstance instance = createInstance();
    Device device = createDevice(instance);
    Window window = createWindow(device, instance, 800, 600, "vulkan renderer");

    TextRenderer text_renderer = createTextRenderer(device, windowPipelineConfig(window));
    TextFont text_font = createTextFont(device, &window.device_loop);

    while (!glfwWindowShouldClose(window.window)) {
        glfwPollEvents();

        u32 image = beginWindowFrame(&window, device, (VkClearColorValue){0, 20, 255, 0});

        drawTextFont(currentCommand(window), device, text_renderer, text_font);

        endWindowFrame(&window, device, image);
    }

    vkDeviceWaitIdle(device.logical);

    destroyTextFont(device, text_font);
    destroyTextRenderer(device, text_renderer);
    destroyWindow(window, device, instance);
    destroyDevice(device, instance);
    vkDestroyInstance(instance, NULL);
    glfwTerminate();
    return 0;
}
