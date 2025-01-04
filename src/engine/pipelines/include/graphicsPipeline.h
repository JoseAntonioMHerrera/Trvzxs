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
void createSyncObjectsForGraphicAndCompute(GraphicPipelineToSurface *gd, VkDevice *device);
void startRenderPass(GraphicPipelineToSurface *gdp, GraphicPipelineToSurface *particleGraphics, DisplayManager *dm, VkFramebuffer swapchainFramebuffer);
 void recordGraphicsCommandBuffer(GraphicPipelineToSurface *gdp, GraphicPipelineToSurface* particleGraphics, DisplayManager* dm, uint32_t imageIndex, PhysicalDevice* pd);
VkShaderModule* createShaderModule(VkDevice device, char** code, size_t codeSize);


void fillObjectBindingDescriptions(VkVertexInputBindingDescription **objectIBD, VkVertexInputRate inputRate, size_t stride);
void fillObjectInputAttributeDescription(VkVertexInputAttributeDescription **objectsIAD, uint32_t viadLen);
void fillParticleInputAttributeDescription(VkVertexInputAttributeDescription **objectsIAD, uint32_t viadLen);
void fillParticleBindingDescriptions(VkVertexInputBindingDescription **objectIBD, VkVertexInputRate inputRate, size_t stride);
void createRenderPass(PhysicalDevice *pd, GraphicSwapchain *dsm, VkDevice device, GraphicPipelineToSurface *gdp);
void createParticleRenderPass(PhysicalDevice *pd, GraphicSwapchain *dsm, VkDevice device, GraphicPipelineToSurface *gdp);
void createModelsGraphicPipeline(GraphicPipelineToSurface *gdp, DisplayManager *dm, VkDevice* device);
void createParticlesGraphicPipeline(GraphicPipelineToSurface *gdp, DisplayManager *dm, VkDevice* device);
