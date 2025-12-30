#include "graphics/command.h"
#include "graphics/descriptor.h"
#include "formats/wavefront.h"
#include "graphics/uniform.h"
#include "physics/constraints.h"
#include "physics/mesh_inertia.h"
#include "physics/particle.h"
#include "physics/physics_scene.h"
#include <GLFW/glfw3.h>
#include <cglm/affine-pre.h>
#include <cglm/affine2d.h>
#include <cglm/io.h>
#include <cglm/mat3.h>
#include <cglm/mat4.h>
#include <cglm/quat.h>
#include <cglm/types.h>
#include <elc/core.h>
#include <vulkan/vulkan_core.h>
#define GLM_FORCE_DEPTH_ZERO_TO_ONE 1
#define GLM_FORCE_RADIANS 1
#include "graphics/instance.h"
#include "graphics/device.h"
#include "graphics/window.h"
#include "graphics/model.h"
#include "graphics/texture.h"
#include "graphics/camera.h"
#include "graphics/text.h"
#include "graphics/simple_draw.h"

int main() {
    glfwInit();
    VkInstance instance = createInstance();
    Device device = createDevice(instance);
    Window window = createWindow(device, instance, 800, 600, "vulkan renderer");

    TextRenderer text_renderer = createTextRenderer(device, windowPipelineConfig(window));
    TextFont text_font = createTextFont(device, &window.device_loop, ELC_KILOBYTE, "images/fonts/minogram_6x10.png");

    DiffuseRenderer renderer = createDiffuseRenderer(device, &window.device_loop, windowPipelineConfig(window));

    Camera camera = createCamera();
    glfwSetInputMode(window.window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    Texture texture = loadTexture(device, &window.device_loop, QUEUE_TYPE_GRAPHICS, "images/2K/Poliigon_MetalSteelBrushed_7174_BaseColor.jpg");
    u32 texture_id = addDescriptorTexture(device, &window.device_loop, SAMPLER_LINEAR, texture);

    HostMesh mesh = loadWavefront("models/crankshaft.obj");
    Particle particle = {.rotation = GLM_QUAT_IDENTITY_INIT, .velocity = {1.0f}};
    vec3 mass_center;
    computeMeshInertia(mesh, 1.0f, particle.inertia, mass_center, &particle.mass);
    particle.position[1] = mass_center[1];
    particle.omega[0] = 0.1f;
    particle.mass = 1.0f;
    shiftMeshBackwards(mesh, mass_center);
    Model model = hostMeshToModel(device, mesh);
    destroyHostMesh(mesh);
    Particle crankshaft = particle;
    Particle engine_base = {.position = {0.0f, 5.0f, 0.0f}, .mass = 0.0f, .inertia = GLM_MAT3_IDENTITY_INIT, .rotation = GLM_QUAT_IDENTITY_INIT};

    while (!glfwWindowShouldClose(window.window)) {
        glfwPollEvents();

        updateCamera(&camera, window);

        applyParticleGravity(&engine_base, 1.0f / 2400.0f);
        applyParticleGravity(&crankshaft, 1.0f / 2400.0f);
        for (u32 i = 0; i < 1; i++) {
            BallJoint constraint = createBallJoint(engine_base, crankshaft, (vec3){0.0f, -5.0f, 0.0f}, (vec3){0.0f, -mass_center[1], 0.0f}, 1.0f / 2400.0f);
            solveBallJoint(constraint, &engine_base, &crankshaft);
        }
        applyParticleVelocity(&engine_base, 1.0f / 2400.0f);
        applyParticleVelocity(&crankshaft, 1.0f / 2400.0f);

        glm_vec3_print(crankshaft.position, stdout);

        u32 image = beginWindowFrame(&window, device);
        beginWindowPass(window, image, (vec4){0.25f, 0.25f, 0.25f, 0.0f});

        setRendererCamera(device, renderer, camera);
        vkCmdBindPipeline(currentCommand(window), VK_PIPELINE_BIND_POINT_GRAPHICS, renderer.pipeline);

        mat4 transform;
        glm_translate_make(transform, crankshaft.position);
        glm_quat_rotate(transform, crankshaft.rotation, transform);
        drawTexturedModel(currentCommand(window), renderer, device, model, texture_id, transform);

        drawTextFont(currentCommand(window), device, text_renderer, &text_font, windowAspect(window));

        endWindowFrame(&window, device, image);
    }

    vkDeviceWaitIdle(device.logical);

    destroyTexture(device, texture);
    destroyModel(device, model);
    destroyDiffuseRenderer(device, renderer);
    destroyTextFont(device, text_font);
    destroyTextRenderer(device, text_renderer);
    destroyWindow(window, device, instance);
    destroyDevice(device, instance);
    vkDestroyInstance(instance, NULL);
    glfwTerminate();
    return 0;
}
