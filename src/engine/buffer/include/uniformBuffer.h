#pragma once
#include <time.h>
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

MODEL_VIEW_PROJECTION updateUniformBuffers(GraphicSwapchain* dsm);

int createUniformBuffers(PhysicalDevice *pd, LogicalDevice *ld, const uint32_t ubosLen, UniformBufferManager *ubos);
