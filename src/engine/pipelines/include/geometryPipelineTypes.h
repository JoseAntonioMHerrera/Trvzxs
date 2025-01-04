#pragma once
#include <vulkan/vulkan.h>
#include "vertex.h"
#include "bufferTypes.h"
#include "textureManagerTypes.h"
#include "scene.h"
#include "computePipeline.h"


typedef struct DeviceBufferManager DeviceBufferManager;
typedef struct CommandPools CommandPools;
typedef struct ComputePipeline ComputePipeline;
typedef struct TextureManager {
    Texture2D* p_texture;
    VkDescriptorSet *descriptorSet;
} TextureManager;

typedef struct GraphicPipelineToSurface {
    uint32_t maxFrames;

    size_t commandLen;
    VkRenderPass* renderPass;
    SCENE* geometry;
    Texture2D* texture;
    DeviceBufferManager *deviceBuffer;

    VkPipeline *pipeline;

    // LAYOUT //
    VkPipelineLayout *pipelineLayout;

    // SWAPCHAIN //
    GraphicSwapchain *swapchain;

    // DESCRIPTORS //
    VkDescriptorSetLayout dsl;
    VkDescriptorPool descriptorPool;
    VkDescriptorSet* descriptorSet;

    // COMMANDS //
    CommandPools *commandPool;
    VkCommandBuffer *commandBuffer;


    // ATTACHTMENTS //
    VkVertexInputBindingDescription *objectIBD;
    size_t objectIBDLen;
    VkVertexInputAttributeDescription *objectIAD;
    size_t objectIADLen;

    // SYNC //
    VkSemaphore *renderEndFlag;
    VkSemaphore *imgViewSemaphore;
    VkFence *inFlightFence;

    // SHADERS //
    VkShaderModule *vertShader;
    VkShaderModule *fragShader;
    VkShaderModule *geomShader;

    // Compute pipeline for graphics operations

    ComputePipeline* computeP;

} GraphicPipelineToSurface;
