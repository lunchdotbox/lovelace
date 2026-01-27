#include "instance.h"

#include <elc/core.h>

VkInstance createInstance() {
    u32 extension_count;
    const char** extensions = glfwGetRequiredInstanceExtensions(&extension_count);

    VkApplicationInfo app = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "hyperplane",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "tgvge",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_API_VERSION_1_4,
    };

    VkInstanceCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &app,
        .enabledExtensionCount = extension_count,
        .ppEnabledExtensionNames = extensions,
    };

    VkInstance instance;
    vkCreateInstance(&info, NULL, &instance);

    return instance;
}
