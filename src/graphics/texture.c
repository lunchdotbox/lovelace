#include "texture.h"

#include "command.h"
#include "buffer.h"
#include "stb_image.h"
#include "device_loop.h"

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

void uploadImageData(Device device, QueueType type, VkExtent3D extent, void* data, u64 data_size, VkImage image) {
    VkBuffer staging_buffer = createBuffer(device, data_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, false);
    VkDeviceMemory staging_memory = createBufferMemory(device, staging_buffer, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    void* staging_buffer_mapped;
    vkMapMemory(device.logical, staging_memory, 0, data_size, 0, &staging_buffer_mapped);
    memcpy(staging_buffer_mapped, data, data_size);
    vkUnmapMemory(device.logical, staging_memory);

    VkCommandBuffer command = beginSingleTimeCommands(device, type);
    imageMemoryBarrier(command, VK_IMAGE_LAYOUT_UNDEFINED, IMAGE_LAYOUT_TRANSFER_DST, 0, VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 1, image);
    commandCopyBufferToImage(command, extent, staging_buffer, image);
    imageMemoryBarrier(command, IMAGE_LAYOUT_TRANSFER_DST, IMAGE_LAYOUT_SHADER_READ, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 1, image);
    endSingleTimeCommands(device, type, command);

    vkFreeMemory(device.logical, staging_memory, NULL);
    vkDestroyBuffer(device.logical, staging_buffer, NULL);
}

Texture loadTexture(Device device, DeviceLoop* loop, QueueType type, const char* path) {
    int width, height, channels;
    stbi_set_flip_vertically_on_load(true);
    stbi_uc* pixels = stbi_load(path, &width, &height, &channels, STBI_rgb_alpha);

    Texture texture = createTexture(device, (VkExtent2D){.width = width, .height = height});
    uploadImageData(device, type, (VkExtent3D){.width = width, .height = height, .depth = 1}, pixels, width * height * 4, texture.color_image);

    stbi_image_free(pixels);
    return texture;
}
