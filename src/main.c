#define GLM_FORCE_DEPTH_ZERO_TO_ONE 1
#define GLM_FORCE_RADIANS 1
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <vulkan/vulkan_core.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <elc/core.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define DEFAULT_PIPELINE_CONFIG(config) {\
    config = (PipelineConfig){};\
    config.primitive_topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;\
    config.cull_mode = VK_CULL_MODE_NONE;\
    config.enable_depth = VK_TRUE;\
    config.depth_op = VK_COMPARE_OP_LESS;\
    config.bindings = &(VkVertexInputBindingDescription){.binding = 0, .stride = sizeof(VulkanVertex), .inputRate = VK_VERTEX_INPUT_RATE_VERTEX};\
    config.n_bindings = 1;\
    config.color_attachments = &(VkPipelineColorBlendAttachmentState){\
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,\
        .blendEnable = VK_FALSE,\
    };\
    config.n_color_attachments = 1;\
    config.attributes = (VkVertexInputAttributeDescription[]){\
        {.binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .location = 0, .offset = offsetof(VulkanVertex, position)},\
        {.binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .location = 1, .offset = offsetof(VulkanVertex, normal)},\
        {.binding = 0, .format = VK_FORMAT_R32G32_SFLOAT, .location = 2, .offset = offsetof(VulkanVertex, uv)},\
    };\
    config.n_attributes = 3;\
}

#define FRAMES_IN_FLIGHT 2

typedef enum QueueType {
    QUEUE_TYPE_GRAPHICS,
    QUEUE_TYPE_PRESENT,
    QUEUE_TYPE_COMPUTE,
} QueueType;

#define IMAGE_LAYOUT_SHADER_READ (VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
#define IMAGE_LAYOUT_TRANSFER_DST (VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)

typedef struct DeviceLoop {
    VkDescriptorSet descriptor_sets[FRAMES_IN_FLIGHT];
    VkCommandBuffer commands[FRAMES_IN_FLIGHT];
    VkSemaphore semaphores[FRAMES_IN_FLIGHT];
    VkFence fences[FRAMES_IN_FLIGHT];
    u32 frame, set_index;
    bool written;
} DeviceLoop;

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

typedef struct Device {
    VkPhysicalDevice physical;
    VkDevice logical;
    VkSampler sampler;
    VkPipelineLayout pipeline_layout;
    VkDescriptorPool descriptor_pool;
    VkDescriptorSetLayout set_layout;
    VkQueue graphics_queue, present_queue, compute_queue;
    VkCommandPool graphics_pool, present_pool, compute_pool;
    u32 graphics_family, present_family, compute_family;
} Device;

typedef struct PipelineConfig {
    VkRenderPass render_pass;
    VkShaderModule fragment, vertex, control, evaluation;
    VkPipelineColorBlendAttachmentState* color_attachments;
    VkVertexInputAttributeDescription* attributes;
    VkVertexInputBindingDescription* bindings;
    VkPolygonMode polygon_mode;
    VkCullModeFlags cull_mode;
    VkBool32 enable_depth;
    VkCompareOp depth_op;
    VkExtent2D extent;
    VkPrimitiveTopology primitive_topology;
    u32 n_bindings, n_attributes, patch_points, n_color_attachments;
} PipelineConfig;

typedef struct Model {
    VkBuffer vertex_buffer, index_buffer;
    VkDeviceMemory vertex_memory, index_memory;
    u32 vertex_count, index_count;
} Model;

typedef struct Texture {
    VkImage color_image;
    VkDeviceMemory color_memory;
    VkImageView color_view;
} Texture;

typedef struct VulkanVertex {
    vec3 position, normal;
    vec2 uv;
} VulkanVertex;

typedef struct BoundingBox {
    vec3 max, min;
} BoundingBox;

typedef struct PushConstantData {
    mat4 view_matrix, model_matrix;
} PushConstantData;

typedef struct Camera {
    double last_mouse_x, last_mouse_y;
    vec3 position, rotation;
    mat4 projection;
} Camera;

VkQueueFamilyProperties* getPhysicalDeviceQueueFamilyProperties(VkInstance instance, VkPhysicalDevice physical_device, u32* n_properties) {
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, n_properties, NULL);
    VkQueueFamilyProperties* properties = malloc(*n_properties * sizeof(VkQueueFamilyProperties));
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, n_properties, properties);
    return properties;
}

bool getPhysicalDeviceSupport(VkSurfaceKHR surface, VkPhysicalDevice device, u32 queue_family_index) {
    VkBool32 support;
    vkGetPhysicalDeviceSurfaceSupportKHR(device, queue_family_index, surface, &support);
    return support != VK_FALSE;
}

VkPhysicalDeviceFeatures getPhysicalDeviceFeatures(VkPhysicalDevice device) {
    VkPhysicalDeviceFeatures result;
    vkGetPhysicalDeviceFeatures(device, &result);

    return result;
}

void pickPhysicalDevice(VkInstance instance, VkSurfaceKHR surface, Device* device) {
    u32 n_physical_devices;
    vkEnumeratePhysicalDevices(instance, &n_physical_devices, NULL);
    VkPhysicalDevice* physical_devices = malloc(n_physical_devices * sizeof(VkPhysicalDevice));
    vkEnumeratePhysicalDevices(instance, &n_physical_devices, physical_devices);

    for (u64 i = 0; i < n_physical_devices; i++) {
        VkPhysicalDevice physical_device = physical_devices[i];

        VkPhysicalDeviceFeatures features = getPhysicalDeviceFeatures(physical_device);
        u32 n_properties;
        VkQueueFamilyProperties* properties = getPhysicalDeviceQueueFamilyProperties(instance, physical_device, &n_properties);

        u32 graphics_family_index, present_family_index, compute_family_index;
        bool has_graphics_family = false, has_present_family = false, has_compute_family = false;

        for (u64 i = 0; i < n_properties; i++) {
            VkQueueFamilyProperties property = properties[i];

            bool is_surface_supported = true;
            if (surface) is_surface_supported = getPhysicalDeviceSupport(surface, physical_device, i);

            if (is_surface_supported) {
                present_family_index = i;
                has_present_family = true;
            }

            if ((property.queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0) {
                graphics_family_index = i;
                has_graphics_family = true;
            }

            if ((property.queueFlags & VK_QUEUE_COMPUTE_BIT) != 0) {
                compute_family_index = i;
                has_compute_family = true;
            }

            if (has_graphics_family && has_present_family && has_compute_family && features.samplerAnisotropy == VK_TRUE && features.tessellationShader == VK_TRUE && features.fillModeNonSolid == VK_TRUE) {
                device->physical = physical_device;
                device->graphics_family = graphics_family_index;
                device->present_family = present_family_index;
                device->compute_family = compute_family_index;

                free(physical_devices);
                free(properties);

                return;
            }
        }

        free(properties);
    }

    free(physical_devices);
    device->physical = VK_NULL_HANDLE;
}

void createLogicalDevice(VkInstance instance, Device* device) {
    float queue_priorities[1] = {1.0f};

    VkDeviceQueueCreateInfo queue_create_infos[3] = {
        {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = device->graphics_family,
            .queueCount = ARRAY_LENGTH(queue_priorities),
            .pQueuePriorities = queue_priorities,
        },
        {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = device->present_family,
            .queueCount = ARRAY_LENGTH(queue_priorities),
            .pQueuePriorities = queue_priorities,
        },
        {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = device->compute_family,
            .queueCount = ARRAY_LENGTH(queue_priorities),
            .pQueuePriorities = queue_priorities,
        },
    };

    const char* extensions[1] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

    VkPhysicalDeviceDescriptorIndexingFeatures indexing = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES,
        .shaderSampledImageArrayNonUniformIndexing = VK_TRUE,
        .runtimeDescriptorArray = VK_TRUE,
        .descriptorBindingVariableDescriptorCount = VK_TRUE,
        .descriptorBindingPartiallyBound = VK_TRUE,
    };

    VkPhysicalDeviceFeatures enabled_features = {
        .samplerAnisotropy = VK_TRUE,
        .fillModeNonSolid = VK_TRUE,
        .tessellationShader = VK_TRUE,
        .shaderSampledImageArrayDynamicIndexing = VK_TRUE,
    };

    VkDeviceCreateInfo device_create_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = &indexing,
        .queueCreateInfoCount = ARRAY_LENGTH(queue_create_infos),
        .pQueueCreateInfos = queue_create_infos,
        .enabledExtensionCount = ARRAY_LENGTH(extensions),
        .ppEnabledExtensionNames = extensions,
        .pEnabledFeatures = &enabled_features,
    };
    if (device->graphics_family == device->present_family) device_create_info.queueCreateInfoCount -= 1;
    if (device->present_family == device->compute_family) device_create_info.queueCreateInfoCount -= 1;

    vkCreateDevice(device->physical, &device_create_info, NULL, &device->logical);

    vkGetDeviceQueue(device->logical, device->graphics_family, 0, &device->graphics_queue);
    vkGetDeviceQueue(device->logical, device->present_family, 0, &device->present_queue);
    vkGetDeviceQueue(device->logical, device->compute_family, 0, &device->compute_queue);
}

VkCommandPool createCommandPool(Device device, u32 queue_family) {
    VkCommandPoolCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = queue_family,
    };

    VkCommandPool command_pool;
    vkCreateCommandPool(device.logical, &info, NULL, &command_pool);

    return command_pool;
}

VkDescriptorPool createDescriptorPool(Device device, u32 set_count, u32 descriptor_count) {
    VkDescriptorPoolCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
        .pPoolSizes = (VkDescriptorPoolSize[]){
            {.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = set_count * descriptor_count},
            {.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .descriptorCount = set_count * descriptor_count},
        },
        .poolSizeCount = 2,
        .maxSets = set_count,
    };

    VkDescriptorPool descriptor_pool;
    vkCreateDescriptorPool(device.logical, &info, NULL, &descriptor_pool);

    return descriptor_pool;
}

VkDescriptorSetLayout createSetLayout(Device device, u32 descriptor_count) {
    VkDescriptorSetLayoutBinding bindings[] = {
        (VkDescriptorSetLayoutBinding){.binding = 0, .descriptorCount = descriptor_count, .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT},
    };

    VkDescriptorBindingFlags flags[] = {
        VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT,
    };

    VkDescriptorSetLayoutBindingFlagsCreateInfo flags_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
        .bindingCount = ARRAY_LENGTH(flags),
        .pBindingFlags = flags,
    };

    VkDescriptorSetLayoutCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = &flags_info,
        .bindingCount = ARRAY_LENGTH(bindings),
        .pBindings = bindings,
    };

    VkDescriptorSetLayout set_layout;
    vkCreateDescriptorSetLayout(device.logical, &info, NULL, &set_layout);

    return set_layout;
}

void allocateDescriptorSets(Device device, VkDescriptorSetLayout layout, u32 count, VkDescriptorSet* descriptor_sets) {
    VkDescriptorSetLayout layouts[count];
    for (u32 i = 0; i < count; i++) layouts[i] = layout;

    VkDescriptorSetAllocateInfo info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = device.descriptor_pool,
        .descriptorSetCount = count,
        .pSetLayouts = layouts,
    };

    vkAllocateDescriptorSets(device.logical, &info, descriptor_sets);
}

void freeDescriptorSets(Device device, u32 count, VkDescriptorSet* descriptor_sets) {
    vkFreeDescriptorSets(device.logical, device.descriptor_pool, count, descriptor_sets);
}

VkPipelineLayout createPipelineLayout(Device device, u64 push_size, VkShaderStageFlags push_stage) {
    VkPushConstantRange push_constant_range = {
        .stageFlags = push_stage,
        .offset = 0,
        .size = push_size,
    };

    VkPipelineLayoutCreateInfo layout_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &push_constant_range,
        .setLayoutCount = 1,
        .pSetLayouts = &device.set_layout,
    };
    if (push_size == 0) {
        layout_create_info.pushConstantRangeCount = 0;
        layout_create_info.pPushConstantRanges = NULL;
    }

    VkPipelineLayout layout;
    vkCreatePipelineLayout(device.logical, &layout_create_info, NULL, &layout);

    return layout;
}

VkCommandPool queueTypeToPool(Device device, QueueType type) {
    switch (type) {
        case QUEUE_TYPE_GRAPHICS: return device.graphics_pool;
        case QUEUE_TYPE_PRESENT: return device.present_pool;
        case QUEUE_TYPE_COMPUTE: return device.compute_pool;
    }
    return NULL;
}

void freeCommandBuffers(Device device, QueueType type, u32 count, VkCommandBuffer* result) {
    vkFreeCommandBuffers(device.logical, queueTypeToPool(device, type), count, result);
}

void allocateCommandBuffers(Device device, QueueType type, u32 count, VkCommandBuffer* result) {
    VkCommandBufferAllocateInfo info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandPool = queueTypeToPool(device, type),
        .commandBufferCount = count,
    };

    vkAllocateCommandBuffers(device.logical, &info, result);
}

VkSemaphore createSemaphore(Device device) {
    VkSemaphoreCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    };

    VkSemaphore semaphore;
    vkCreateSemaphore(device.logical, &info, NULL, &semaphore);

    return semaphore;
}

VkFence createFence(Device device) {
    VkFenceCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT,
    };

    VkFence fence;
    vkCreateFence(device.logical, &info, NULL, &fence);

    return fence;
}

DeviceLoop createDeviceLoop(Device device, QueueType type) {
    DeviceLoop loop = {0};
    allocateDescriptorSets(device, device.set_layout, FRAMES_IN_FLIGHT, loop.descriptor_sets);
    allocateCommandBuffers(device, type, FRAMES_IN_FLIGHT, loop.commands);
    for (u32 i = 0; i < FRAMES_IN_FLIGHT; i++) {
        loop.semaphores[i] = createSemaphore(device);
        loop.fences[i] = createFence(device);
    }
    return loop;
}

void destroyDeviceLoop(DeviceLoop loop, Device device, QueueType type) {
    for (u32 i = 0; i < FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(device.logical, loop.semaphores[i], NULL);
        vkDestroyFence(device.logical, loop.fences[i], NULL);
    }
    freeCommandBuffers(device, type, FRAMES_IN_FLIGHT, loop.commands);
    freeDescriptorSets(device, FRAMES_IN_FLIGHT, loop.descriptor_sets);
}

VkSampler createSampler(Device device, bool linear, VkCompareOp compare_op, VkSamplerAddressMode address_mode) {
    VkSamplerCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .addressModeU = address_mode,
        .addressModeV = address_mode,
        .addressModeW = address_mode,
        .magFilter = VK_FILTER_NEAREST,
        .minFilter = VK_FILTER_NEAREST,
        .anisotropyEnable = VK_TRUE,
        .maxAnisotropy = 1.0,
        .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
        .unnormalizedCoordinates = VK_FALSE,
        .compareEnable = compare_op != VK_COMPARE_OP_ALWAYS,
        .compareOp = compare_op,
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .mipLodBias = 0.0,
        .minLod = 0.0,
        .maxLod = 0.0,
    };
    if (linear) {
        create_info.magFilter = VK_FILTER_LINEAR;
        create_info.minFilter = VK_FILTER_LINEAR;
    }

    VkSampler sampler;
    vkCreateSampler(device.logical, &create_info, NULL, &sampler);

    return sampler;
}

Device createDevice(VkInstance instance) {
    Device device;
    pickPhysicalDevice(instance, NULL, &device);
    createLogicalDevice(instance, &device);
    device.descriptor_pool = createDescriptorPool(device, FRAMES_IN_FLIGHT * 2, 20000);
    device.set_layout = createSetLayout(device, 10000);
    device.pipeline_layout = createPipelineLayout(device, sizeof(PushConstantData), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
    device.graphics_pool = createCommandPool(device, device.graphics_family);
    device.present_pool = createCommandPool(device, device.present_family);
    device.compute_pool = createCommandPool(device, device.compute_family);
    device.sampler = createSampler(device, true, VK_COMPARE_OP_ALWAYS, VK_SAMPLER_ADDRESS_MODE_REPEAT);
    return device;
}

void destroyDevice(Device device, VkInstance instance) {
    vkDeviceWaitIdle(device.logical);
    vkDestroySampler(device.logical, device.sampler, NULL);
    vkDestroyCommandPool(device.logical, device.graphics_pool, NULL);
    vkDestroyCommandPool(device.logical, device.present_pool, NULL);
    vkDestroyCommandPool(device.logical, device.compute_pool, NULL);
    vkDestroyPipelineLayout(device.logical, device.pipeline_layout, NULL);
    vkDestroyDescriptorSetLayout(device.logical, device.set_layout, NULL);
    vkDestroyDescriptorPool(device.logical, device.descriptor_pool, NULL);
    vkDestroyDevice(device.logical, NULL);
}

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

VkFormat findSupportedFormat(Device device, VkFormat* candidates, u32 n_candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
    for (u64 i = 0; i < n_candidates; i++) {
        VkFormat candidate = candidates[i];

        VkFormatProperties properties;
        vkGetPhysicalDeviceFormatProperties(device.physical, candidate, &properties);

        if (tiling == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & features) == features) return candidate;
        if (tiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & features) == features) return candidate;
    }

    return VK_FORMAT_UNDEFINED;
}

u32 findMemoryType(Device device, u32 filter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memory_properties;
    vkGetPhysicalDeviceMemoryProperties(device.physical, &memory_properties);
    for (u32 i = 0; i < memory_properties.memoryTypeCount; i++)
        if ((filter & ((u32)1 << i)) != 0 && (memory_properties.memoryTypes[i].propertyFlags & properties) == properties)
            return i;

    return UINT32_MAX;
}

VkDeviceMemory allocateDeviceMemory(Device device, VkMemoryPropertyFlags properties, VkMemoryRequirements requirements) {
    u32 memory_type = findMemoryType(device, requirements.memoryTypeBits, properties);

    VkMemoryAllocateInfo allocate_info = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = requirements.size,
        .memoryTypeIndex = memory_type,
    };

    VkDeviceMemory memory;
    vkAllocateMemory(device.logical, &allocate_info, NULL, &memory);

    return memory;
}

VkImage createImage(Device device, VkExtent3D extent, VkImageType type, VkFormat format, VkImageUsageFlags usage, VkImageCreateFlags flags, bool linear_tiling, u32 n_layers) {
    VkImageCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = type,
        .extent = extent,
        .mipLevels = 1,
        .arrayLayers = n_layers,
        .format = format,
        .tiling = linear_tiling ? VK_IMAGE_TILING_LINEAR : VK_IMAGE_TILING_OPTIMAL,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .usage = usage,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .flags = flags,
    };

    VkImage image;
    vkCreateImage(device.logical, &create_info, NULL, &image);

    return image;
}

VkImageView createImageView(Device device, VkImage image, VkImageViewType type, VkFormat format, VkImageAspectFlags image_aspect, u32 n_layers) {
    VkImageViewCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = image,
        .viewType = type,
        .format = format,
        .subresourceRange = {
            .aspectMask = image_aspect,
            .baseMipLevel = 0,
            .baseArrayLayer = 0,
            .levelCount = 1,
            .layerCount = n_layers,
        },
    };

    VkImageView view;
    vkCreateImageView(device.logical, &create_info, NULL, &view);

    return view;
}

VkDeviceMemory createImageMemory(Device device, VkImage image, VkMemoryPropertyFlags properties) {
    VkMemoryRequirements requirements;
    vkGetImageMemoryRequirements(device.logical, image, &requirements);

    VkDeviceMemory memory = allocateDeviceMemory(device, properties, requirements);
    vkBindImageMemory(device.logical, image, memory, 0);

    return memory;
}

VkFramebuffer createFramebuffer(Device device, VkRenderPass render_pass, VkImageView* attachments, u32 n_attachments, u32 width, u32 height, u32 layers) {
    VkFramebufferCreateInfo framebuffer_create_info = {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = render_pass,
        .attachmentCount = n_attachments,
        .pAttachments = attachments,
        .width = width,
        .height = height,
        .layers = layers,
    };

    VkFramebuffer framebuffer;
    vkCreateFramebuffer(device.logical, &framebuffer_create_info, NULL, &framebuffer);

    return framebuffer;
}

VkRenderPass createRenderPass(Device device, VkFormat depth_format, VkFormat color_format) {
    VkAttachmentDescription depth_attachment = {
        .format = depth_format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };

    VkAttachmentReference depth_attachment_reference = {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };

    VkAttachmentDescription color_attachment = {
        .format = color_format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    };

    VkAttachmentReference color_attachment_reference = {
        .attachment = 1,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };

    VkSubpassDescription subpass = {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &color_attachment_reference,
        .pDepthStencilAttachment = &depth_attachment_reference,
    };

    VkSubpassDependency dependency = {
        .dstSubpass = 0,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .srcAccessMask = 0,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
    };

    VkAttachmentDescription attachments[] = {depth_attachment, color_attachment};

    VkRenderPassCreateInfo render_pass_create_info = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = ARRAY_LENGTH(attachments),
        .pAttachments = attachments,
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = &dependency,
    };

    VkRenderPass render_pass;
    vkCreateRenderPass(device.logical, &render_pass_create_info, NULL, &render_pass);

    return render_pass;
}

void createSwapchain(Device device, VkExtent2D extent, Window* window) {
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

VkDescriptorSet getLoopSet(DeviceLoop loop) {
    return loop.descriptor_sets[loop.frame];
}

void propogateDescriptorWrites(Device device, DeviceLoop* loop) {
    if (loop->written) {
        VkCopyDescriptorSet copy = {
            .sType = VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET,
            .srcSet = getLoopSet(*loop),
            .dstSet = loop->descriptor_sets[(loop->frame + 1) % FRAMES_IN_FLIGHT],
            .descriptorCount = 10000,
        };

        vkUpdateDescriptorSets(device.logical, 0, NULL, 1, &copy);
    }
    loop->written = false;
}

void beginDeviceLoop(Device device, DeviceLoop* loop) {
    vkWaitForFences(device.logical, 1, &loop->fences[loop->frame], VK_TRUE, UINT64_MAX);
    propogateDescriptorWrites(device, loop);
    vkResetFences(device.logical, 1, &loop->fences[loop->frame]);
}

VkSemaphore getLoopSemaphore(DeviceLoop loop) {
    return loop.semaphores[loop.frame];
}

VkFence getLoopFence(DeviceLoop loop) {
    return loop.fences[loop.frame];
}

u32 acquireNextSwapchainImage(Device device, Window* window) {
    beginDeviceLoop(device, &window->device_loop);

    u32 result;
    vkAcquireNextImageKHR(device.logical, window->swapchain, UINT64_MAX, getLoopSemaphore(window->device_loop), VK_NULL_HANDLE, &result);

    return result;
}

void submitCommandBuffer(Device device, DeviceLoop* loop, VkCommandBuffer command, VkSemaphore signal) {
    VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &(VkSemaphore){getLoopSemaphore(*loop)},
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &signal,
        .commandBufferCount = 1,
        .pCommandBuffers = &command,
        .pWaitDstStageMask = (VkShaderStageFlags[]){VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT},
    };

    vkQueueSubmit(device.graphics_queue, 1, &submit_info, getLoopFence(*loop));
    loop->frame = (loop->frame + 1) % FRAMES_IN_FLIGHT;
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

void beginCommandBuffer(VkCommandBuffer buffer) {
    VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    };

    vkBeginCommandBuffer(buffer, &begin_info);
}

void beginRenderPass(VkRenderPass render_pass, VkFramebuffer framebuffer, VkExtent2D extent, VkCommandBuffer buffer, VkClearColorValue* clear_color, float clear_depth) {
    VkClearValue clear_values[2] = {
        {
            .depthStencil = { .depth = clear_depth, .stencil = 0 },
        },
    };
    if (clear_color) clear_values[1].color = *clear_color;

    VkRenderPassBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = render_pass,
        .framebuffer = framebuffer,
        .renderArea = {
            .offset = { .x = 0, .y = 0 },
            .extent = extent,
        },
        .clearValueCount = clear_color ? 2 : 1,
        .pClearValues = clear_values,
    };

    vkCmdBeginRenderPass(buffer, &begin_info, VK_SUBPASS_CONTENTS_INLINE);
}

VkCommandBuffer currentCommand(Window window) {
    return window.device_loop.commands[window.device_loop.frame];
}

u32 beginWindowFrame(Window* window, Device device, VkClearColorValue clear) {
    u32 image = acquireNextSwapchainImage(device, window);
    vkResetCommandBuffer(currentCommand(*window), 0);
    beginCommandBuffer(currentCommand(*window));
    beginRenderPass(window->render_pass, window->framebuffers[image], window->extent, currentCommand(*window), &clear, 1.0f);
    vkCmdBindDescriptorSets(currentCommand(*window), VK_PIPELINE_BIND_POINT_GRAPHICS, device.pipeline_layout, 0, 1, &(VkDescriptorSet){getLoopSet(window->device_loop)}, 0, NULL);
    return image;
}

void endWindowFrame(Window* window, Device device, u32 image) {
    VkCommandBuffer command = currentCommand(*window);
    vkCmdEndRenderPass(command);
    vkEndCommandBuffer(command);
    submitAndPresent(device, window, command, image);
}

void readFile(const char* path, u8** data, u64* size) {
    FILE* file = fopen(path, "r");
    if (file == NULL) {
        *data = NULL;
        return;
    }

    fseek(file, 0, SEEK_END);
    *size = ftell(file);
    rewind(file);

    *data = malloc(*size);
    fread(*data, 1, *size, file);

    fclose(file);
}

VkShaderModule createShaderModule(Device device, const char* file_path) {
    u32* code;
    u64 code_size;
    readFile(file_path, (u8**)&code, &code_size);

    VkShaderModuleCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = code_size,
        .pCode = code,
    };

    VkShaderModule shader_module;
    vkCreateShaderModule(device.logical, &create_info, NULL, &shader_module);

    free(code);
    return shader_module;
}

void destroyPipelineConfig(PipelineConfig config, Device device) {
    if (config.fragment) vkDestroyShaderModule(device.logical, config.fragment, NULL);
    if (config.vertex) vkDestroyShaderModule(device.logical, config.vertex, NULL);
    if (config.control) vkDestroyShaderModule(device.logical, config.control, NULL);
    if (config.evaluation) vkDestroyShaderModule(device.logical, config.fragment, NULL);
}

VkPipeline createPipeline(Device device, PipelineConfig config) {
    VkPipelineInputAssemblyStateCreateInfo input_assembly_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = config.primitive_topology,
        .primitiveRestartEnable = VK_FALSE,
    };

    VkViewport viewport = {
        .x = 0.0f,
        .y = 0.0f,
        .width = config.extent.width,
        .height = config.extent.height,
        .minDepth = 0.0,
        .maxDepth = 1.0,
    };

    VkRect2D scissor = {
        .offset = {.x = 0, .y = 0},
        .extent = config.extent,
    };

    VkPipelineViewportStateCreateInfo viewport_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .pViewports = &viewport,
        .scissorCount = 1,
        .pScissors = &scissor,
    };

    VkPipelineRasterizationStateCreateInfo rasterization_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = config.polygon_mode,
        .lineWidth = 1.0,
        .cullMode = config.cull_mode,
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
    };

    VkPipelineMultisampleStateCreateInfo multisample_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .sampleShadingEnable = VK_FALSE,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
    };

    VkPipelineColorBlendStateCreateInfo color_blend_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .attachmentCount = config.n_color_attachments,
        .pAttachments = config.color_attachments,
    };

    VkPipelineDepthStencilStateCreateInfo depth_stencil_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = config.enable_depth,
        .depthWriteEnable = config.enable_depth,
        .depthCompareOp = config.depth_op,
        .depthBoundsTestEnable = VK_FALSE,
    };

    VkPipelineVertexInputStateCreateInfo vertex_input_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexAttributeDescriptionCount = config.n_attributes,
        .pVertexAttributeDescriptions = config.attributes,
        .vertexBindingDescriptionCount = config.n_bindings,
        .pVertexBindingDescriptions = config.bindings,
    };

    VkPipelineTessellationStateCreateInfo tessellation_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO,
        .patchControlPoints = config.patch_points,
    };

    VkPipelineShaderStageCreateInfo shader_stages[4] = {
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = config.vertex,
            .pName = "main",
        },
    };

    u32 stage_count = 1;

    if (config.fragment) shader_stages[stage_count++] = (VkPipelineShaderStageCreateInfo){
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        .module = config.fragment,
        .pName = "main",
    };

    if (config.control) {
        shader_stages[stage_count++] = (VkPipelineShaderStageCreateInfo){
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT,
            .module = config.control,
            .pName = "main",
        };

        shader_stages[stage_count++] = (VkPipelineShaderStageCreateInfo){
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT,
            .module = config.evaluation,
            .pName = "main",
        };
    }

    VkGraphicsPipelineCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = stage_count,
        .pStages = shader_stages,
        .pInputAssemblyState = &input_assembly_create_info,
        .pViewportState = &viewport_create_info,
        .pRasterizationState = &rasterization_create_info,
        .pMultisampleState = &multisample_create_info,
        .pColorBlendState = &color_blend_create_info,
        .pDepthStencilState = &depth_stencil_create_info,
        .pVertexInputState = &vertex_input_create_info,
        .pTessellationState = config.control ? &tessellation_create_info : NULL,
        .pDynamicState = NULL,
        .layout = device.pipeline_layout,
        .renderPass = config.render_pass,
        .subpass = 0,
        .basePipelineIndex = -1,
        .basePipelineHandle = VK_NULL_HANDLE,
    };

    VkPipeline pipeline;
    vkCreateGraphicsPipelines(device.logical, VK_NULL_HANDLE, 1, &create_info, NULL, &pipeline);

    destroyPipelineConfig(config, device);
    return pipeline;
}

void setPipelineVertexShader(PipelineConfig* config, VkShaderModule shader) {
    config->vertex = shader;
}

void setPipelineFragmentShader(PipelineConfig* config, VkShaderModule shader) {
    config->fragment = shader;
}

void setPipelineExtent(PipelineConfig* config, VkExtent2D extent) {
    config->extent = extent;
}

void setPipelineRenderPass(PipelineConfig* config, VkRenderPass render_pass) {
    config->render_pass = render_pass;
}

void setPipelineWindow(PipelineConfig* config, Window window) {
    setPipelineExtent(config, window.extent);
    setPipelineRenderPass(config, window.render_pass);
}

VkBuffer createBuffer(Device device, VkDeviceSize size, VkBufferUsageFlags usage, bool concurrent) {
    VkBufferCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = usage,
        .sharingMode = concurrent ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE,
    };

    VkBuffer buffer;
    vkCreateBuffer(device.logical, &create_info, NULL, &buffer);

    return buffer;
}

VkDeviceMemory createBufferMemory(Device device, VkBuffer buffer, VkMemoryPropertyFlags properties) {
    VkMemoryRequirements requirements;
    vkGetBufferMemoryRequirements(device.logical, buffer, &requirements);

    VkDeviceMemory memory = allocateDeviceMemory(device, properties, requirements);
    vkBindBufferMemory(device.logical, buffer, memory, 0);

    return memory;
}

BoundingBox boundingBoxCube1x1x1() {
    return (BoundingBox){
        .max = {0.5f, 0.5f, 0.5f},
        .min = {-0.5f, -0.5f, -0.5f},
    };
}

void boundingBoxTranslate(BoundingBox* bounding_box, vec3 vector) {
    glm_vec3_add(bounding_box->max, vector, bounding_box->max);
    glm_vec3_add(bounding_box->min, vector, bounding_box->min);
}

bool boundingBoxOverlapping(BoundingBox bounding_box, BoundingBox other) {
    return bounding_box.min[0] <= other.max[0] &&
        bounding_box.max[0] >= other.min[0] &&
        bounding_box.min[1] <= other.max[1] &&
        bounding_box.max[1] >= other.min[1] &&
        bounding_box.min[2] <= other.max[2] &&
        bounding_box.max[2] >= other.min[2];
}

void boundingBoxVertices(BoundingBox self, VulkanVertex* vertices) {
    vec3 normals[] = {
        { -1.0f, 0.0f, 0.0f },
        { 1.0f, 0.0f, 0.0f },
        { 0.0f, -1.0f, 0.0f },
        { 0.0f, 1.0f, 0.0f },
        { 0.0f, 0.0f, 1.0f },
        { 0.0f, 0.0f, -1.0f },
    };

    vec2 quad_uvs[] = {
        { 0.0f, 0.0f },
        { 1.0f, 1.0f },
        { 0.0f, 1.0f },
        { 1.0f, 0.0f },
    };

    vertices[0] = (VulkanVertex){ .position = { self.min[0], self.min[1], self.min[2] }, .normal = { normals[0][0], normals[0][1], normals[0][2] }, .uv = { quad_uvs[0][0], quad_uvs[0][1] } };
    vertices[1] = (VulkanVertex){ .position = { self.min[0], self.max[1], self.max[2] }, .normal = { normals[0][0], normals[0][1], normals[0][2] }, .uv = { quad_uvs[1][0], quad_uvs[1][1] } };
    vertices[2] = (VulkanVertex){ .position = { self.min[0], self.min[1], self.max[2] }, .normal = { normals[0][0], normals[0][1], normals[0][2] }, .uv = { quad_uvs[2][0], quad_uvs[2][1] } };
    vertices[3] = (VulkanVertex){ .position = { self.min[0], self.max[1], self.min[2] }, .normal = { normals[0][0], normals[0][1], normals[0][2] }, .uv = { quad_uvs[3][0], quad_uvs[3][1] } };

    vertices[4] = (VulkanVertex){ .position = { self.max[0], self.min[1], self.min[2] }, .normal = { normals[1][0], normals[1][1], normals[1][2] }, .uv = { quad_uvs[0][0], quad_uvs[0][1] } };
    vertices[5] = (VulkanVertex){ .position = { self.max[0], self.max[1], self.max[2] }, .normal = { normals[1][0], normals[1][1], normals[1][2] }, .uv = { quad_uvs[1][0], quad_uvs[1][1] } };
    vertices[6] = (VulkanVertex){ .position = { self.max[0], self.min[1], self.max[2] }, .normal = { normals[1][0], normals[1][1], normals[1][2] }, .uv = { quad_uvs[2][0], quad_uvs[2][1] } };
    vertices[7] = (VulkanVertex){ .position = { self.max[0], self.max[1], self.min[2] }, .normal = { normals[1][0], normals[1][1], normals[1][2] }, .uv = { quad_uvs[3][0], quad_uvs[3][1] } };

    vertices[8] = (VulkanVertex){ .position = { self.min[0], self.min[1], self.min[2] }, .normal = { normals[2][0], normals[2][1], normals[2][2] }, .uv = { quad_uvs[0][0], quad_uvs[0][1] } };
    vertices[9] = (VulkanVertex){ .position = { self.max[0], self.min[1], self.max[2] }, .normal = { normals[2][0], normals[2][1], normals[2][2] }, .uv = { quad_uvs[1][0], quad_uvs[1][1] } };
    vertices[10] = (VulkanVertex){ .position = { self.min[0], self.min[1], self.max[2] }, .normal = { normals[2][0], normals[2][1], normals[2][2] }, .uv = { quad_uvs[2][0], quad_uvs[2][1] } };
    vertices[11] = (VulkanVertex){ .position = { self.max[0], self.min[1], self.min[2] }, .normal = { normals[2][0], normals[2][1], normals[2][2] }, .uv = { quad_uvs[3][0], quad_uvs[3][1] } };

    vertices[12] = (VulkanVertex){ .position = { self.min[0], self.max[1], self.min[2] }, .normal = { normals[3][0], normals[3][1], normals[3][2] }, .uv = { quad_uvs[0][0], quad_uvs[0][1] } };
    vertices[13] = (VulkanVertex){ .position = { self.max[0], self.max[1], self.max[2] }, .normal = { normals[3][0], normals[3][1], normals[3][2] }, .uv = { quad_uvs[1][0], quad_uvs[1][1] } };
    vertices[14] = (VulkanVertex){ .position = { self.min[0], self.max[1], self.max[2] }, .normal = { normals[3][0], normals[3][1], normals[3][2] }, .uv = { quad_uvs[2][0], quad_uvs[2][1] } };
    vertices[15] = (VulkanVertex){ .position = { self.max[0], self.max[1], self.min[2] }, .normal = { normals[3][0], normals[3][1], normals[3][2] }, .uv = { quad_uvs[3][0], quad_uvs[3][1] } };

    vertices[16] = (VulkanVertex){ .position = { self.min[0], self.min[1], self.max[2] }, .normal = { normals[4][0], normals[4][1], normals[4][2] }, .uv = { quad_uvs[0][0], quad_uvs[0][1] } };
    vertices[17] = (VulkanVertex){ .position = { self.max[0], self.max[1], self.max[2] }, .normal = { normals[4][0], normals[4][1], normals[4][2] }, .uv = { quad_uvs[1][0], quad_uvs[1][1] } };
    vertices[18] = (VulkanVertex){ .position = { self.min[0], self.max[1], self.max[2] }, .normal = { normals[4][0], normals[4][1], normals[4][2] }, .uv = { quad_uvs[2][0], quad_uvs[2][1] } };
    vertices[19] = (VulkanVertex){ .position = { self.max[0], self.min[1], self.max[2] }, .normal = { normals[4][0], normals[4][1], normals[4][2] }, .uv = { quad_uvs[3][0], quad_uvs[3][1] } };

    vertices[20] = (VulkanVertex){ .position = { self.min[0], self.min[1], self.min[2] }, .normal = { normals[5][0], normals[5][1], normals[5][2] }, .uv = { quad_uvs[0][0], quad_uvs[0][1] } };
    vertices[21] = (VulkanVertex){ .position = { self.max[0], self.max[1], self.min[2] }, .normal = { normals[5][0], normals[5][1], normals[5][2] }, .uv = { quad_uvs[1][0], quad_uvs[1][1] } };
    vertices[22] = (VulkanVertex){ .position = { self.min[0], self.max[1], self.min[2] }, .normal = { normals[5][0], normals[5][1], normals[5][2] }, .uv = { quad_uvs[2][0], quad_uvs[2][1] } };
    vertices[23] = (VulkanVertex){ .position = { self.max[0], self.min[1], self.min[2] }, .normal = { normals[5][0], normals[5][1], normals[5][2] }, .uv = { quad_uvs[3][0], quad_uvs[3][1] } };
}

void boundingBoxIndices(BoundingBox bounding_box, u32* indices) {
    memcpy(indices, (u32[]){0, 1, 2, 0, 3, 1, 4, 5, 6, 4, 7, 5, 8, 9, 10, 8, 11, 9, 12, 13, 14, 12, 15, 13, 16, 17, 18, 16, 19, 17, 20, 21, 22, 20, 23, 21}, sizeof(u32) * 36);
}

Model createModel(Device device, u32 vertex_size, u32 vertex_count, u32 index_count, void** vertex_mapped, void** index_mapped) {
    Model model;
    model.vertex_count = vertex_count;
    model.index_count = index_count;

    model.vertex_buffer = createBuffer(device, vertex_size * vertex_count, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, false);
    model.vertex_memory = createBufferMemory(device, model.vertex_buffer, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    model.index_buffer = createBuffer(device, sizeof(u32) * index_count, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, false);
    model.index_memory = createBufferMemory(device, model.index_buffer, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    vkMapMemory(device.logical, model.vertex_memory, 0, vertex_size * vertex_count, 0, vertex_mapped);
    vkMapMemory(device.logical, model.index_memory, 0, sizeof(u32) * index_count, 0, index_mapped);

    return model;
}

VkQueue queueTypeToQueue(Device device, QueueType type) {
    switch (type) {
        case QUEUE_TYPE_GRAPHICS: return device.graphics_queue;
        case QUEUE_TYPE_PRESENT: return device.present_queue;
        case QUEUE_TYPE_COMPUTE: return device.compute_queue;
    }
    return NULL;
}

VkCommandBuffer beginSingleTimeCommands(Device device, QueueType type) {
    VkCommandBuffer command;
    allocateCommandBuffers(device, type, 1, &command);

    VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };

    vkBeginCommandBuffer(command, &begin_info);

    return command;
}

void endSingleTimeCommands(Device device, QueueType type, VkCommandBuffer command_buffer) {
    vkEndCommandBuffer(command_buffer);

    VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &command_buffer,
    };

    vkQueueSubmit(queueTypeToQueue(device, type), 1, &submit_info, VK_NULL_HANDLE);
    vkQueueWaitIdle(queueTypeToQueue(device, type));

    vkFreeCommandBuffers(device.logical, queueTypeToPool(device, type), 1, &command_buffer);
}

void commandCopyBuffer(VkCommandBuffer command_buffer, VkBuffer src_buffer, VkBuffer dst_buffer, u64 copy_size) {
    VkBufferCopy copy_region = {.size = copy_size};
    vkCmdCopyBuffer(command_buffer, src_buffer, dst_buffer, 1, &copy_region);
}

void copyBuffer(Device device, QueueType type, VkBuffer src_buffer, VkBuffer dst_buffer, u64 copy_size) {
    VkCommandBuffer command = beginSingleTimeCommands(device, type);

    commandCopyBuffer(command, src_buffer, dst_buffer, copy_size);

    endSingleTimeCommands(device, type, command);
}

void destroyModel(Device device, Model model) {
    vkFreeMemory(device.logical, model.index_memory, NULL);
    vkDestroyBuffer(device.logical, model.index_buffer, NULL);

    vkFreeMemory(device.logical, model.vertex_memory, NULL);
    vkDestroyBuffer(device.logical, model.vertex_buffer, NULL);
}

void finishModelUpload(Device device, Model* model, u32 vertex_size) {
    vkUnmapMemory(device.logical, model->vertex_memory);
    vkUnmapMemory(device.logical, model->index_memory);

    VkBuffer vertex_buffer = createBuffer(device, vertex_size * model->vertex_count, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, false);
    VkDeviceMemory vertex_memory = createBufferMemory(device, vertex_buffer, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    VkBuffer index_buffer = createBuffer(device, sizeof(u32) * model->index_count, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, false);
    VkDeviceMemory index_memory = createBufferMemory(device, index_buffer, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    VkCommandBuffer command = beginSingleTimeCommands(device, QUEUE_TYPE_GRAPHICS);
    commandCopyBuffer(command, model->vertex_buffer, vertex_buffer, vertex_size * model->vertex_count);
    commandCopyBuffer(command, model->index_buffer, index_buffer, sizeof(u32) * model->index_count);
    endSingleTimeCommands(device, QUEUE_TYPE_GRAPHICS, command);

    destroyModel(device, *model);

    model->index_buffer = index_buffer;
    model->index_memory = index_memory;
    model->vertex_buffer = vertex_buffer;
    model->vertex_memory = vertex_memory;
}

Model boundingBoxModel(Device device, BoundingBox bounding_box) {
    void* vertex_mapped, *index_mapped;
    Model model = createModel(device, sizeof(VulkanVertex), 24, 36, &vertex_mapped, &index_mapped);

    boundingBoxVertices(bounding_box, vertex_mapped);
    boundingBoxIndices(bounding_box, index_mapped);

    finishModelUpload(device, &model, sizeof(VulkanVertex));

    return model;
}

PushConstantData getCameraPush(Camera camera) {
    PushConstantData push;
    glm_euler(camera.rotation, push.view_matrix);
    glm_translate(push.view_matrix, camera.position);
    glm_mat4_mul(camera.projection, push.view_matrix, push.view_matrix);
    glm_mat4_identity(push.model_matrix);

    return push;
}

void updateCamera(Camera* camera, Window window) {
    double mouse_x, mouse_y;
    glfwGetCursorPos(window.window, &mouse_x, &mouse_y);

    camera->rotation[0] -= (mouse_y - camera->last_mouse_y) * 0.002;
    camera->rotation[1] += (mouse_x - camera->last_mouse_x) * 0.002;

    camera->rotation[0] = CLAMP(camera->rotation[0], -((GLM_PI / 2.0) - 0.1), (GLM_PI / 2.0) - 0.1);

    vec3 camera_movement = {};

    if (glfwGetKey(window.window, GLFW_KEY_S)) camera_movement[2] -= 1.0f;
    if (glfwGetKey(window.window, GLFW_KEY_W)) camera_movement[2] += 1.0f;
    if (glfwGetKey(window.window, GLFW_KEY_D)) camera_movement[0] -= 1.0f;
    if (glfwGetKey(window.window, GLFW_KEY_A)) camera_movement[0] += 1.0f;

    glm_normalize(camera_movement);
    glm_vec3_rotate(camera_movement, -camera->rotation[1], (vec3){0.0f, 1.0f, 0.0f});
    glm_vec3_scale(camera_movement, 0.001f, camera_movement);
    glm_vec3_add(camera->position, camera_movement, camera->position);

    if (glfwGetKey(window.window, GLFW_KEY_LEFT_SHIFT)) camera->position[1] -= 0.001f;
    if (glfwGetKey(window.window, GLFW_KEY_SPACE)) camera->position[1] += 0.001f;

    camera->last_mouse_x = mouse_x;
    camera->last_mouse_y = mouse_y;
}

Camera createCamera() {
    Camera camera = {0};
    glm_perspective_rh_no(GLM_PI / 4.0f, 800.0f / 600.0f, 0.01f, 1000.0f, camera.projection);
    return camera;
}

void setDescriptorImage(Device device, DeviceLoop* loop, u32 index, VkDescriptorImageInfo image) {
    VkWriteDescriptorSet write = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pImageInfo = &image,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = 1,
        .dstArrayElement = index,
        .dstSet = loop->descriptor_sets[loop->frame],
        .dstBinding = 0,
    };

    vkUpdateDescriptorSets(device.logical, 1, &write, 0, NULL);
    loop->written = true;
}

u32 addDescriptorImage(Device device, DeviceLoop* loop, VkDescriptorImageInfo image) {
    setDescriptorImage(device, loop, loop->set_index, image);
    return loop->set_index++;
}

Texture createTexture(Device device, VkExtent2D extent) {
    Texture texture;
    texture.color_image = createImage(device, (VkExtent3D){extent.width, extent.height, 1}, VK_IMAGE_TYPE_2D, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 0, false, 1);
    texture.color_memory = createImageMemory(device, texture.color_image, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    texture.color_view = createImageView(device, texture.color_image, VK_IMAGE_VIEW_TYPE_2D, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, 1);
    return texture;
}

void destroyTexture(Device device, Texture texture) {
    vkDestroyImageView(device.logical, texture.color_view, NULL);
    vkFreeMemory(device.logical, texture.color_memory, NULL);
    vkDestroyImage(device.logical, texture.color_image, NULL);
}

u32 addDescriptorTexture(Device device, DeviceLoop* loop, Texture texture) {
    VkDescriptorImageInfo info = {
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        .imageView = texture.color_view,
        .sampler = device.sampler,
    };

    return addDescriptorImage(device, loop, info);
}

void copyBufferToImage(VkCommandBuffer command, VkExtent3D extent, VkBuffer buffer, VkImage image) {
    VkBufferImageCopy region = {
        .imageSubresource = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
        .imageExtent = extent,
    };

    vkCmdCopyBufferToImage(command, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}

void imageMemoryBarrier(VkCommandBuffer command, VkImageLayout old, VkImageLayout new, VkAccessFlags src_mask, VkAccessFlags dst_mask, VkPipelineStageFlags src_stage, VkPipelineStageFlags dst_stage, u32 layers, VkImage image) {
    VkImageMemoryBarrier barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .oldLayout = old,
        .newLayout = new,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image,
        .srcAccessMask = src_mask,
        .dstAccessMask = dst_mask,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = layers,
        },
    };

    vkCmdPipelineBarrier(command, src_stage, dst_stage, 0, 0, NULL, 0, NULL, 1, &barrier);
}

void uploadImageData(Device device, QueueType type, VkExtent3D extent, void* data, u64 data_size, VkImage image) {
    VkBuffer staging_buffer = createBuffer(device, data_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, false);
    VkDeviceMemory staging_memory = createBufferMemory(device, staging_buffer, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    void* staging_buffer_mapped;
    vkMapMemory(device.logical, staging_memory, 0, data_size, 0, &staging_buffer_mapped);
    memcpy(staging_buffer_mapped, data, data_size);
    vkUnmapMemory(device.logical, staging_memory);

    VkCommandBuffer command = beginSingleTimeCommands(device, type);
    imageMemoryBarrier(command, VK_IMAGE_LAYOUT_UNDEFINED, IMAGE_LAYOUT_TRANSFER_DST, 0, VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 1, image);
    copyBufferToImage(command, extent, staging_buffer, image);
    imageMemoryBarrier(command, IMAGE_LAYOUT_TRANSFER_DST, IMAGE_LAYOUT_SHADER_READ, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 1, image);
    endSingleTimeCommands(device, type, command);

    vkFreeMemory(device.logical, staging_memory, NULL);
    vkDestroyBuffer(device.logical, staging_buffer, NULL);
}

Texture loadTexture(Device device, DeviceLoop* loop, QueueType type, const char* path) {
    int width, height, channels;
    stbi_uc* pixels = stbi_load(path, &width, &height, &channels, STBI_rgb_alpha);

    Texture texture = createTexture(device, (VkExtent2D){.width = width, .height = height});
    uploadImageData(device, type, (VkExtent3D){.width = width, .height = height, .depth = 1}, pixels, width * height * 4, texture.color_image);

    stbi_image_free(pixels);
    return texture;
}

int main() {
    glfwInit();
    VkInstance instance = createInstance();
    Device device = createDevice(instance);
    Window window = createWindow(device, instance, 800, 600, "vulkan renderer");

    glfwSetInputMode(window.window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    PipelineConfig pipeline_config;
    DEFAULT_PIPELINE_CONFIG(pipeline_config);
    setPipelineWindow(&pipeline_config, window);
    setPipelineVertexShader(&pipeline_config, createShaderModule(device, "spv/terrain.vert.spv"));
    setPipelineFragmentShader(&pipeline_config, createShaderModule(device, "spv/diffuse.frag.spv"));
    VkPipeline pipeline = createPipeline(device, pipeline_config);

    Model model = boundingBoxModel(device, boundingBoxCube1x1x1());

    Texture texture = loadTexture(device, &window.device_loop, QUEUE_TYPE_GRAPHICS, "images/cat.png");
    addDescriptorTexture(device, &window.device_loop, texture);

    Camera camera = createCamera();

    while (!glfwWindowShouldClose(window.window)) {
        updateCamera(&camera, window);

        glfwPollEvents();

        u32 image = beginWindowFrame(&window, device, (VkClearColorValue){0, 20, 255, 0});

        PushConstantData push = getCameraPush(camera);

        vkCmdBindPipeline(currentCommand(window), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
        vkCmdBindVertexBuffers(currentCommand(window), 0, 1, &model.vertex_buffer, (u64[]){0});
        vkCmdBindIndexBuffer(currentCommand(window), model.index_buffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdPushConstants(currentCommand(window), device.pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstantData), &push);
        vkCmdDrawIndexed(currentCommand(window), 36, 1, 0, 0, 0);

        endWindowFrame(&window, device, image);
    }

    vkDeviceWaitIdle(device.logical);

    destroyTexture(device, texture);
    destroyModel(device, model);
    vkDestroyPipeline(device.logical, pipeline, NULL);
    destroyWindow(window, device, instance);
    destroyDevice(device, instance);
    vkDestroyInstance(instance, NULL);
    glfwTerminate();
    return 0;
}
