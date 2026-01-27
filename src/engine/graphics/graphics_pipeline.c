#include "graphics_pipeline.h"

#include "device.h"
#include "window.h"

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
