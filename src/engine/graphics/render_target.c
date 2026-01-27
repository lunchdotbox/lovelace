#include "render_target.h"

#include "device.h"

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
