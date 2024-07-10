#pragma once
#include <vulkan/vulkan.h>
#include "vertex.h"
#include "bufferTypes.h"
typedef struct DeviceBufferManager {
    VkBuffer *buffer;
    VkDeviceMemory *bufferMemory;
    size_t bufferLen;
    VkBuffer *indexBuffer;
    VkDeviceMemory *indexBufferMemory;
    size_t indexLen;
    size_t stagingLen;
    VkBuffer *stagingBuffer;
    VkDeviceMemory *stagingBufferMemory;
    UniformBufferManager *ubo;

} DeviceBufferManager;

typedef struct GeometryDisplayPipelineManager {
    size_t maxFrames;

    size_t commandLen;
    VkRenderPass* renderPass;
    GEOM_OBJECT* geometry;
    DeviceBufferManager *deviceBuffer;

    VkPipeline *pipeline;

    // LAYOUT //
    VkPipelineLayout pipelineLayout;
    // COMMANDS //
    VkCommandPool transferCommandPool;
    VkCommandPool graphicsCommandPool;
    VkCommandBuffer *commandBuffer;

    // ATTACHTMENTS
    VkVertexInputBindingDescription *objectIBD;
    size_t objectIBDLen;
    VkVertexInputAttributeDescription *objectIAD;
    size_t objectIADLen;
    // SYNC //
    VkSemaphore *renderEndFlag;
    VkSemaphore *imgViewSemaphore;
    VkFence *inFlightFence;
    // SHADERS
    VkShaderModule *vertShader;
    VkShaderModule *fragShader;
    VkShaderModule *geomShader;
} GeometryDisplayPipelineManager;
