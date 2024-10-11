#pragma once
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>
#include "transformations.h"
#include "file_functions.h"
#include "buffer.h"
#include "vertex.h"
#include "numberTypes.h"
#include "geometryPipelineTypes.h"
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include "pushConstants.h"
#include "deviceManagmentTypes.h"
#include <assert.h>

#define TRIANGLE_VERT_SHADER "shaders/bin/triangle.vert.spv\0"
#define TRIANGLE_FRAG_SHADER "shaders/bin/triangle.frag.spv\0"

 void createSyncObjects(GraphicPipelineToSurface *gd, VkDevice *device);

 void startRenderPass(GraphicPipelineToSurface *gdp, DisplayManager *dm, VkFramebuffer swapchainFramebuffer);

 void recordCommandBuffer(GraphicPipelineToSurface *gdp, DisplayManager* dm, uint32_t imageIndex);

 VkShaderModule* createShaderModule(VkDevice device, char** code, size_t codeSize);

 void fillObjectBindingDescriptions(VkVertexInputBindingDescription **objectIBD);

 void fillObjectInputAttributeDescription(VkVertexInputAttributeDescription **objectsIAD);

 void createRenderPass(PhysicalDevice *pd, GraphicSwapchain *dsm, VkDevice device, GraphicPipelineToSurface *gdp);

 void createGraphicsPipeline(GraphicPipelineToSurface *gdp, DisplayManager *dm, VkDevice* device);
