#pragma once
#include <vulkan/vulkan.h>
#include <stddef.h>
#include <stdio.h>
#include "transformations.h"
#include "vertex.h"
#include "compilerMacros.h"
#include "numberTypes.h"
#include "miscTypes.h"
#include "deviceManagment.h"
#include "commandBuffer.h"
#include "buffer.h"
#include <string.h> //TODO revisar memcpy de aqui
#include "bufferTypes.h"
#include "cam.h"
#include "mat4.h"
#include "affine.h"
//#include "displayTimer.h"
#include <time.h>

void createDescriptorPool(uint32_t maxFrames, VkDevice device, GraphicPipelineToSurface* gdp);
void createDescriptorSet(uint32_t maxFrames, VkDevice device, GraphicPipelineToSurface* gdp, GraphicPipelineToSurface* particleGraphic, GraphicPipelineToSurface* lightGraphic, Texture2D* texture);
void createDescriptorSetLayout(LogicalDevice *ld, GraphicPipelineToSurface* gdp, uint32_t maxFrames);
void updateGBufferDescriptors(uint32_t maxFrames, VkDevice device, GraphicPipelineToSurface* gdp);
