#ifndef WINDOW_H
#define WINDOW_H

#include "device.h"
#include "device_loop.h"

typedef struct Window {
    GLFWwindow* window;
    VkSurfaceKHR surface;
    VkExtent2D extent;
    VkSwapchainKHR swapchain;
    VkRenderPass render_pass;
    VkImage* color_images, *depth_images;
    VkImageView* color_views, *depth_views;
    VkDeviceMemory* depth_memories;
    VkFramebuffer* framebuffers;
    VkSemaphore* render_semaphores;
    DeviceLoop device_loop;
    u32 image_count;
} Window;

Window createWindow(Device device, VkInstance instance, int width, int height, const char* title);
void destroyWindow(Window window, Device device, VkInstance instance);
u32 acquireNextSwapchainImage(Device device, Window* window);
void submitAndPresent(Device device, Window* window, VkCommandBuffer command, u32 image_index);
VkCommandBuffer currentCommand(Window window);
u32 beginWindowFrame(Window* window, Device device);
void endWindowFrame(Window* window, Device device, u32 image);
float windowAspect(Window window);
void beginWindowPass(Window window, u32 image, vec4 clear);

#endif
