#include "device.h"

#include "descriptor.h"
#include "command.h"
#include "graphics_pipeline.h"
#include "texture.h"
#include <elc/core.h>
#include <vulkan/vulkan_core.h>

ELC_INLINE VkQueueFamilyProperties* getPhysicalDeviceQueueFamilyProperties(VkInstance instance, VkPhysicalDevice physical_device, u32* n_properties) {
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, n_properties, NULL);
    VkQueueFamilyProperties* properties = malloc(*n_properties * sizeof(VkQueueFamilyProperties));
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, n_properties, properties);
    return properties;
}

ELC_INLINE bool getPhysicalDeviceSupport(VkSurfaceKHR surface, VkPhysicalDevice device, u32 queue_family_index) {
    VkBool32 support;
    vkGetPhysicalDeviceSurfaceSupportKHR(device, queue_family_index, surface, &support);
    return support != VK_FALSE;
}

ELC_INLINE VkPhysicalDeviceFeatures getPhysicalDeviceFeatures(VkPhysicalDevice device) {
    VkPhysicalDeviceFeatures result;
    vkGetPhysicalDeviceFeatures(device, &result);

    return result;
}

ELC_INLINE void pickPhysicalDevice(VkInstance instance, VkSurfaceKHR surface, Device* device) {
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

ELC_INLINE void createLogicalDevice(VkInstance instance, Device* device) {
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

VkCommandPool queueTypeToPool(Device device, QueueType type) {
    switch (type) {
        case QUEUE_TYPE_GRAPHICS: return device.graphics_pool;
        case QUEUE_TYPE_PRESENT: return device.present_pool;
        case QUEUE_TYPE_COMPUTE: return device.compute_pool;
    }
    return NULL;
}

VkQueue queueTypeToQueue(Device device, QueueType type) {
    switch (type) {
        case QUEUE_TYPE_GRAPHICS: return device.graphics_queue;
        case QUEUE_TYPE_PRESENT: return device.present_queue;
        case QUEUE_TYPE_COMPUTE: return device.compute_queue;
    }
    return NULL;
}

Device createDevice(VkInstance instance) {
    Device device;
    pickPhysicalDevice(instance, NULL, &device);
    createLogicalDevice(instance, &device);
    device.descriptor_pool = createDescriptorPool(device, 4, 100000);
    device.set_layout = createSetLayout(device, 10000);
    device.pipeline_layout = createPipelineLayout(device, 128, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
    device.graphics_pool = createCommandPool(device, device.graphics_family);
    device.present_pool = createCommandPool(device, device.present_family);
    device.compute_pool = createCommandPool(device, device.compute_family);
    device.samplers[0] = createSampler(device, false, VK_COMPARE_OP_ALWAYS, VK_SAMPLER_ADDRESS_MODE_REPEAT);
    device.samplers[1] = createSampler(device, true, VK_COMPARE_OP_ALWAYS, VK_SAMPLER_ADDRESS_MODE_REPEAT);
    return device;
}

void destroyDevice(Device device, VkInstance instance) {
    vkDeviceWaitIdle(device.logical);
    for (u32 i = 0; i < ARRAY_LENGTH(device.samplers); i++) vkDestroySampler(device.logical, device.samplers[i], NULL);
    vkDestroyCommandPool(device.logical, device.graphics_pool, NULL);
    vkDestroyCommandPool(device.logical, device.present_pool, NULL);
    vkDestroyCommandPool(device.logical, device.compute_pool, NULL);
    vkDestroyPipelineLayout(device.logical, device.pipeline_layout, NULL);
    vkDestroyDescriptorSetLayout(device.logical, device.set_layout, NULL);
    vkDestroyDescriptorPool(device.logical, device.descriptor_pool, NULL);
    vkDestroyDevice(device.logical, NULL);
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

void* mapDeviceMemory(Device device, VkDeviceMemory memory, VkDeviceSize offset, VkDeviceSize range) {
    void* mapped;
    vkMapMemory(device.logical, memory, offset, range, 0, &mapped);
    return mapped;
}
