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
// #include "displayTimer.h"

void                   updateModelUniformBuffersDirect(void* mappedBuffer, SCENE* scene);
LightTransformations*  updateLightUniformBuffers(GraphicSwapchain* dsm, float deltaTime, Camera3D camera, SCENE* scene);
int                    createModelInstanceTransformationsUniformBuffersMapped(PhysicalDevice* pd, LogicalDevice* ld,
                                                                              const uint32_t ubosLen, UniformBufferManager* ubos,
                                                                              uint32_t instancesLen);
int                    createLightInstanceTransformationsUniformBuffersMapped(PhysicalDevice* pd, LogicalDevice* ld,
                                                                              const uint32_t ubosLen, UniformBufferManager* ubos,
                                                                              uint32_t instancesLen);

int  createDeltaTimeUniformBuffers(PhysicalDevice* pd, LogicalDevice* ld, const uint32_t ubosLen,
                                   UniformBufferManager* ubos);
int  createFrameGlobalsUniformBuffersMapped(PhysicalDevice* pd, LogicalDevice* ld, const uint32_t ubosLen,
                                             UniformBufferManager* ubos);
void updateFrameGlobals(FrameGlobals* globals, GraphicSwapchain* dsm, Camera3D camera);
void freeMappedUniformBufferLocalMemory(mat4** instancedTranformations);
