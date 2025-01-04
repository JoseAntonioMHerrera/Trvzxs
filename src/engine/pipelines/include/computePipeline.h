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
//#include "pushConstants.h"
#include "deviceManagmentTypes.h"
#include <assert.h>
#include "deviceManagmentTypes.h"
#include "commandPool.h"

#define PARTICLE_COMPUTE "shaders/bin/particle.comp.spv\0"


typedef struct DeviceBufferManager DeviceBufferManager;
typedef struct DisplayManager DisplayManager;
typedef struct LogicalDevice LogicalDevice;

typedef struct ComputePipeline {
    uint32_t maxFrames;

    DeviceBufferManager *deviceBuffer;

    VkPipeline *pipeline;

    // LAYOUT //
    VkPipelineLayout *pipelineLayout;

    // COMMANDS
    CommandPools *commandPool;
    VkCommandBuffer *commandBuffer;

    // ATTACHTMENTS
    VkVertexInputBindingDescription *objectIBD;
    size_t objectIBDLen;
    VkVertexInputAttributeDescription *objectIAD;
    size_t objectIADLen;

    // SYNC //
    VkSemaphore *computeEndSemaphore;
    VkSemaphore *computeReadySemaphore;
    VkFence *computeInFlightFence;

    // SHADERS
    VkShaderModule *computeShader;
} ComputePipeline;

void createParticleComputePipeline(ComputePipeline *computeP, VkDevice device);
void createModelSkinningComputePipeline(ComputePipeline *computeP, DisplayManager *dm, VkDevice* device);
void createPostProcComputePipeline(ComputePipeline *computeP, DisplayManager *dm, VkDevice* device);
void recordParticleCommandBuffer(ComputePipeline* computeP, PhysicalDevice *pd, LogicalDevice* ld, VkPipelineLayout layout, VkDescriptorSet *descriptorSet, uint32_t imageIndex, float deltaTime);
