#pragma once
#include <vulkan/vulkan.h>
#include <stddef.h>
#include <stdio.h>
#include "transformations.h"
#include "vertex.h"
#include "compilerMacros.h"
#include "numberTypes.h"
#include "miscTypes.h"
#include "physicalDevice.h"
#include <string.h> //TODO revisar memcpy de aqui

typedef struct GraphicPipelineToSurface GraphicPipelineToSurface;
typedef struct ComputePipeline ComputePipeline;
typedef struct CommandPools {
    VkCommandPool transferCommandPool;
    VkCommandPool graphicsCommandPool;
    VkCommandPool computeCommandPool;
    // Add other command pools as needed
} CommandPools;
void createGraphicsCommandPool(PhysicalDevice* pd, VkDevice device, GraphicPipelineToSurface *gdp);
void createTransferQueueCommandPool(PhysicalDevice* pd, VkDevice device, GraphicPipelineToSurface *gdp);
void createComputeQueueCommandPool(PhysicalDevice* pd, VkDevice device, ComputePipeline *compP);
