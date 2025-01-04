#pragma once
#include <vulkan/vulkan.h>
#include <stddef.h>
#include <stdio.h>
#include "transformations.h"
#include "image.h"
#include "vertex.h"
#include "compilerMacros.h"
#include "numberTypes.h"
#include "miscTypes.h"
#include "deviceManagment.h"
#include "geometryPipelineTypes.h"
#include <string.h> //TODO revisar memcpy de aqui

typedef struct GraphicPipelineToSurface GraphicPipelineToSurface;

VkCommandBuffer beginSingleTimeCommands(LogicalDevice *ld, VkCommandPool commandPool);
void endSingleTimeCommands(VkCommandBuffer commandBuffer, VkCommandPool commandPool, VkQueue queue, VkDevice device);
void copyBufferToImage(LogicalDevice *ld, VkDevice device, VkCommandPool commandPool, VkQueue queue, VkBuffer buffer,
                       VkImage image, uint32_t width, uint32_t height);
void copyBuffer(LogicalDevice *ld, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, VkCommandPool commandPool, VkQueue queue);
void createCommandBuffer(GraphicPipelineToSurface *gdp, VkDevice *device, size_t framesInFlight);
void createCommandBufferWithCompute(GraphicPipelineToSurface *gdp, VkDevice *device, size_t framesInFlight);
