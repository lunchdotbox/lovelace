#ifndef ENGINE_GRAPHICS_TEXTURE_H
#define ENGINE_GRAPHICS_TEXTURE_H

#include "device.h"
#include "device_loop.h"

#define IMAGE_LAYOUT_SHADER_READ (VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
#define IMAGE_LAYOUT_TRANSFER_DST (VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)

typedef struct Texture {
    VkImage color_image;
    VkDeviceMemory color_memory;
    VkImageView color_view;
} Texture;

VkSampler createSampler(Device device, bool linear, VkCompareOp compare_op, VkSamplerAddressMode address_mode);
VkImage createImage(Device device, VkExtent3D extent, VkImageType type, VkFormat format, VkImageUsageFlags usage, VkImageCreateFlags flags, bool linear_tiling, u32 n_layers);
VkImageView createImageView(Device device, VkImage image, VkImageViewType type, VkFormat format, VkImageAspectFlags image_aspect, u32 n_layers);
VkDeviceMemory createImageMemory(Device device, VkImage image, VkMemoryPropertyFlags properties);
Texture createTexture(Device device, VkExtent2D extent);
void destroyTexture(Device device, Texture texture);
void uploadImageData(Device device, QueueType type, VkExtent3D extent, void* data, u64 data_size, VkImage image);
Texture loadTexture(Device device, DeviceLoop* loop, QueueType type, const char* path);

#endif
