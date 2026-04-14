#pragma once
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>
#include "transformations.h"
#include "file_functions.h"
#include "buffer.h"
#include "vertex.h"
#include "numberTypes.h"
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
// #include "pushConstants.h"
#include "deviceManagmentTypes.h"
#include <assert.h>
#include "deviceManagmentTypes.h"
#include "commandPool.h"
#include "bindlessTypes.h"

#define PARTICLE_COMPUTE "shaders/bin/particle.comp.spv\0"
#define OCL_CULLING_COMPUTE "shaders/bin/oclCulling.comp.spv\0"
typedef struct DeviceBufferManager DeviceBufferManager;
typedef struct DisplayManager      DisplayManager;
typedef struct LogicalDevice       LogicalDevice;

typedef struct ComputePipeline
{
    uint32_t maxFrames;

    DeviceBufferManager* deviceBuffer;

    VkPipeline* pipeline;

    // LAYOUT //
    VkPipelineLayout* pipelineLayout;

    // COMMANDS
    CommandPools*    commandPool;
    VkCommandBuffer* commandBuffer;

    // ATTACHTMENTS
    VkVertexInputBindingDescription*   objectIBD;
    size_t                             objectIBDLen;
    VkVertexInputAttributeDescription* objectIAD;
    size_t                             objectIADLen;

    // SHADERS
    VkShaderModule* computeShader;

    // A11 bindless — compute-side wiring. Populated by
    // initBindlessRegistration, consumed by compute pipeline layout
    // creation and the dispatch callback.
    VkDescriptorSetLayout bindlessGlobalLayout;
    VkDescriptorSet       bindlessGlobalSet;
    VkDescriptorSetLayout bindlessPerPassLayout;  // particleComputePass layout
    VkDescriptorPool      perPassPool;
    VkDescriptorSet*      perPassSets;            // maxFrames
} ComputePipeline;
void createOcclusionCullingComputePipeline(ComputePipeline* computeP, VkDevice device);
void createParticleComputePipeline(ComputePipeline* computeP, VkDevice device);
void createModelSkinningComputePipeline(ComputePipeline* computeP, DisplayManager* dm, VkDevice* device);
void createPostProcComputePipeline(ComputePipeline* computeP, DisplayManager* dm, VkDevice* device);
