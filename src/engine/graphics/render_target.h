#ifndef ENGINE_GRAPHICS_RENDER_TARGET_H
#define ENGINE_GRAPHICS_RENDER_TARGET_H

#include "device.h"

VkFramebuffer createFramebuffer(Device device, VkRenderPass render_pass, VkImageView* attachments, u32 n_attachments, u32 width, u32 height, u32 layers);
VkRenderPass createRenderPass(Device device, VkFormat depth_format, VkFormat color_format);
void beginRenderPass(VkRenderPass render_pass, VkFramebuffer framebuffer, VkExtent2D extent, VkCommandBuffer buffer, VkClearColorValue* clear_color, float clear_depth);

#endif
