#include "window.h"

#include "device.h"
#include "render_target.h"
#include "texture.h"
#include "synchronization.h"
#include "command.h"
#include <math.h>

ELC_INLINE void createSwapchain(Device device, VkExtent2D extent, Window* window) {
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device.physical, window->surface, &capabilities);

    u32 n_formats;
    VkSurfaceFormatKHR* formats = NULL;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device.physical, window->surface, &n_formats, NULL);
    if (n_formats != 0) {
        formats = malloc(n_formats * sizeof(VkSurfaceFormatKHR));
        vkGetPhysicalDeviceSurfaceFormatsKHR(device.physical, window->surface, &n_formats, formats);
    }

    u32 n_present_modes;
    VkPresentModeKHR* present_modes = NULL;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device.physical, window->surface, &n_present_modes, NULL);
    if (n_present_modes != 0) {
        present_modes = malloc(n_present_modes * sizeof(VkPresentModeKHR));
        vkGetPhysicalDeviceSurfacePresentModesKHR(device.physical, window->surface, &n_present_modes, present_modes);
    }

    VkFormat depth_format_candidates[] = {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT};
    VkFormat depth_format = findSupportedFormat(device, depth_format_candidates, 3, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);

    VkSurfaceFormatKHR surface_format = formats[0];
    for (u64 i = 0; i < n_formats; i++) {
        VkSurfaceFormatKHR available_format = formats[i];
        if (available_format.format == VK_FORMAT_R8G8B8A8_SRGB && available_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            surface_format = available_format;
            break;
        }
    }

    VkPresentModeKHR present_mode = present_modes[0];
    for (u64 i = 0; i < n_present_modes; i++) {
        VkPresentModeKHR available_present_mode = present_modes[i];
        if (available_present_mode == VK_PRESENT_MODE_MAILBOX_KHR) {
            present_mode = available_present_mode;
            break;
        }
    }

    if (capabilities.currentExtent.width == UINT32_MAX) {
        window->extent = extent;
        window->extent.width = MAX(capabilities.minImageExtent.width, MIN(capabilities.maxImageExtent.width, window->extent.width));
        window->extent.height = MAX(capabilities.minImageExtent.height, MIN(capabilities.maxImageExtent.height, window->extent.height));
    } else window->extent = capabilities.currentExtent;

    u32 queue_family_indices[2] = {device.graphics_family, device.present_family};

    VkSwapchainCreateInfoKHR create_info = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = window->surface,
        .presentMode = present_mode,
        .preTransform = capabilities.currentTransform,
        .imageFormat = surface_format.format,
        .imageColorSpace = surface_format.colorSpace,
        .clipped = VK_TRUE,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .pQueueFamilyIndices = queue_family_indices,
        .imageArrayLayers = 1,
        .imageExtent = window->extent,
    };

    // incase both queues are the same family
    if (queue_family_indices[0] == queue_family_indices[1]) {
        create_info.queueFamilyIndexCount = 1;
        create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    } else {
        create_info.queueFamilyIndexCount = ARRAY_LENGTH(queue_family_indices);
        create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    }

    // if there is no limit on max images just pick one higher than min images
    if (capabilities.maxImageCount == 0)
        create_info.minImageCount = capabilities.minImageCount + 1;
    else
        create_info.minImageCount = MIN(capabilities.minImageCount + 1, capabilities.maxImageCount);

    vkCreateSwapchainKHR(device.logical, &create_info, NULL, &window->swapchain);

    window->render_pass = createRenderPass(device, depth_format, surface_format.format);

    // gets the number of images in the swapchain
    vkGetSwapchainImagesKHR(device.logical, window->swapchain, &window->image_count, NULL);

    // terrible horrible no good pointer slicing
    window->color_images = malloc((sizeof(VkImage) * 10) * window->image_count);
    window->depth_images = window->color_images + (window->image_count);
    window->color_views = (VkImageView*)window->color_images + (window->image_count * 2);
    window->depth_views = (VkImageView*)window->color_images + (window->image_count * 3);
    window->depth_memories = (VkDeviceMemory*)window->color_images + (window->image_count * 4);
    window->framebuffers = (VkFramebuffer*)window->color_images + (window->image_count * 5);
    window->render_semaphores = (VkSemaphore*)window->color_images + (window->image_count * 6);

    vkGetSwapchainImagesKHR(device.logical, window->swapchain, &window->image_count, window->color_images);

    for (u32 i = 0; i < window->image_count; i++) {
        window->depth_images[i] = createImage(device, (VkExtent3D){window->extent.width, window->extent.height, 1}, VK_IMAGE_TYPE_2D, depth_format, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, 0, false, 1);
        window->depth_memories[i] = createImageMemory(device, window->depth_images[i], VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        window->color_views[i] = createImageView(device, window->color_images[i], VK_IMAGE_VIEW_TYPE_2D, create_info.imageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
        window->depth_views[i] = createImageView(device, window->depth_images[i], VK_IMAGE_VIEW_TYPE_2D, depth_format, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
        window->framebuffers[i] = createFramebuffer(device, window->render_pass, (VkImageView[]){window->depth_views[i], window->color_views[i]}, 2, window->extent.width, window->extent.height, 1);
        window->render_semaphores[i] = createSemaphore(device);
    }

    if (n_present_modes != 0) free(present_modes);
    if (n_formats != 0) free(formats);
}

Window createWindow(Device device, VkInstance instance, int width, int height, const char* title) {
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    Window window = {.window = glfwCreateWindow(width, height, title, NULL, NULL)};
    glfwCreateWindowSurface(instance, window.window, NULL, &window.surface);
    createSwapchain(device, (VkExtent2D){.width = width, .height = height}, &window);
    window.device_loop = createDeviceLoop(device, QUEUE_TYPE_GRAPHICS);
    return window;
}

void destroyWindow(Window window, Device device, VkInstance instance) {
    vkDeviceWaitIdle(device.logical);
    for (u32 i = 0; i < window.image_count; i++) {
        vkDestroyFramebuffer(device.logical, window.framebuffers[i], NULL);
        vkDestroyImageView(device.logical, window.color_views[i], NULL);
        vkDestroyImageView(device.logical, window.depth_views[i], NULL);
        vkDestroyImage(device.logical, window.depth_images[i], NULL);
        vkFreeMemory(device.logical, window.depth_memories[i], NULL);
        vkDestroySemaphore(device.logical, window.render_semaphores[i], NULL);
    }
    free(window.color_images);
    destroyDeviceLoop(window.device_loop, device, QUEUE_TYPE_GRAPHICS);
    vkDestroyRenderPass(device.logical, window.render_pass, NULL);
    vkDestroySwapchainKHR(device.logical, window.swapchain, NULL);
    vkDestroySurfaceKHR(instance, window.surface, NULL);
    glfwDestroyWindow(window.window);
}

u32 acquireNextSwapchainImage(Device device, Window* window) {
    beginDeviceLoop(device, &window->device_loop);

    u32 result;
    vkAcquireNextImageKHR(device.logical, window->swapchain, UINT64_MAX, getLoopSemaphore(window->device_loop), VK_NULL_HANDLE, &result);

    return result;
}

void submitAndPresent(Device device, Window* window, VkCommandBuffer command, u32 image_index) {
    VkSemaphore signal_semaphore = window->render_semaphores[image_index];

    submitCommandBuffer(device, &window->device_loop, command, signal_semaphore);

    VkPresentInfoKHR present_info = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &signal_semaphore,
        .swapchainCount = 1,
        .pSwapchains = &window->swapchain,
        .pImageIndices = &image_index,
    };

    vkQueuePresentKHR(device.present_queue, &present_info);
}

VkCommandBuffer currentCommand(Window window) {
    return window.device_loop.commands[window.device_loop.frame];
}

u32 beginWindowFrame(Window* window, Device device) {
    u32 image = acquireNextSwapchainImage(device, window);
    vkResetCommandBuffer(currentCommand(*window), 0);
    beginCommandBuffer(currentCommand(*window));
    vkCmdBindDescriptorSets(currentCommand(*window), VK_PIPELINE_BIND_POINT_GRAPHICS, device.pipeline_layout, 0, 1, &(VkDescriptorSet){getLoopSet(window->device_loop)}, 0, NULL);
    return image;
}

void endWindowFrame(Window* window, Device device, u32 image) {
    VkCommandBuffer command = currentCommand(*window);
    vkCmdEndRenderPass(command);
    vkEndCommandBuffer(command);
    submitAndPresent(device, window, command, image);
}

float windowAspect(Window window) {
    return (float)window.extent.width / (float)window.extent.height;
}

void beginWindowPass(Window window, u32 image, vec4 clear) {
    beginRenderPass(window.render_pass, window.framebuffers[image], window.extent, currentCommand(window), &(VkClearColorValue)VEC4_USE(clear), 1.0f);
}
