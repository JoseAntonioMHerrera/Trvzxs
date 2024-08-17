#pragma once
#include <vulkan/vulkan.h>
#include "vertex.h"
#include "bufferTypes.h"
#include "textureManagerTypes.h"
#include "scene.h"
typedef struct CommandPools {
    VkCommandPool transferCommandPool;
    VkCommandPool graphicsCommandPool;
    // Add other command pools as needed
} CommandPools;
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


typedef struct TextureManager {
    Texture2D* p_texture;
    VkDescriptorSet *descriptorSet;
} TextureManager;

typedef struct GeometryDisplayPipelineManager {
    uint32_t maxFrames;

    size_t commandLen;
    VkRenderPass* renderPass;
    SCENE* geometry;
    Texture2D* texture;
    DeviceBufferManager *deviceBuffer;

    VkPipeline *pipeline;

    // LAYOUT //
    VkPipelineLayout *pipelineLayout;

    // DESCRIPTORS
    VkDescriptorSetLayout dsl;
    VkDescriptorPool descriptorPool;
    VkDescriptorSet* descriptorSet;
    // COMMANDS //
    CommandPools *commandPool;
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
