#include "model.h"

#include "buffer.h"
#include "command.h"
#include "graphics_pipeline.h"
#include "window.h"

static const PipelineConfig model_pipeline_config = MODEL_PIPELINE_CONFIG;

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

PipelineConfig windowPipelineConfig(Window window) {
    PipelineConfig config = model_pipeline_config;
    setPipelineWindow(&config, window);
    return config;
}
