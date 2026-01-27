#ifndef ENGINE_GRAPHICS_DEVICE_H
#define ENGINE_GRAPHICS_DEVICE_H

#include <elc/core.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

typedef struct Device {
    VkPhysicalDevice physical;
    VkDevice logical;
    VkSampler samplers[2];
    VkPipelineLayout pipeline_layout;
    VkDescriptorPool descriptor_pool;
    VkDescriptorSetLayout set_layout;
    VkQueue graphics_queue, present_queue, compute_queue;
    VkCommandPool graphics_pool, present_pool, compute_pool;
    u32 graphics_family, present_family, compute_family;
} Device;

typedef enum QueueType {
    QUEUE_TYPE_GRAPHICS,
    QUEUE_TYPE_PRESENT,
    QUEUE_TYPE_COMPUTE,
} QueueType;

typedef enum Sampler {
    SAMPLER_NEAREST,
    SAMPLER_LINEAR,
} Sampler;

Device createDevice(VkInstance instance);
void destroyDevice(Device device, VkInstance instance);
VkCommandPool queueTypeToPool(Device device, QueueType type);
VkQueue queueTypeToQueue(Device device, QueueType type);
VkFormat findSupportedFormat(Device device, VkFormat* candidates, u32 n_candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
u32 findMemoryType(Device device, u32 filter, VkMemoryPropertyFlags properties);
VkDeviceMemory allocateDeviceMemory(Device device, VkMemoryPropertyFlags properties, VkMemoryRequirements requirements);
void* mapDeviceMemory(Device device, VkDeviceMemory memory, VkDeviceSize offset, VkDeviceSize range);
void flushDeviceMemory(Device device, VkDeviceMemory memory, VkDeviceSize offset, VkDeviceSize size);

#endif
