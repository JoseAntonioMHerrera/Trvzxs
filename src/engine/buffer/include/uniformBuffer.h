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
#include "camera.h"
//#include "displayTimer.h"

MODEL_VIEW_PROJECTION* updateUniformBuffers(GraphicSwapchain* dsm, float deltaTime, Camera3D camera, SCENE* scene);

int createModelInstanceTransformationsUniformBuffersMapped(PhysicalDevice *pd, LogicalDevice *ld, const uint32_t ubosLen, UniformBufferManager *ubos, uint32_t instancesLen);
int createDeltaTimeUniformBuffers(PhysicalDevice *pd, LogicalDevice *ld, const uint32_t ubosLen, UniformBufferManager *ubos);
void freeMappedUniformBufferLocalMemory(mat4** instancedTranformations);
