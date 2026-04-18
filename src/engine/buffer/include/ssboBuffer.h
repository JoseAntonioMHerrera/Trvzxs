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
// #include "displayTimer.h"

int createParticleSSBOBuffer(PhysicalDevice* pd, LogicalDevice* ld, uint32_t ssbosLen, SSBOBufferManager* ssbos,
                             CommandPools* commandPool, SCENE* scene);
int createReadbackLightPass1SSBOBuffers(PhysicalDevice* pd, LogicalDevice* ld, uint32_t ssbosLen,
                                        SSBOBufferManager* ssbos, CommandPools* commandPool);

// Clustered forward+: build the view-space cluster AABB grid from the current
// projection and upload one device-local copy per frame-in-flight. The grid
// is static until the next swapchain recreate; use uploadClusterAABBs() to
// refresh after a resize/FOV change without reallocating.
int createClusterAABBSSBOs(PhysicalDevice* pd, LogicalDevice* ld, uint32_t ssbosLen, SSBOBufferManager* ssbos,
                           CommandPools* commandPool, mat4 proj, float nearZ, float farZ);

// Rebuild cluster AABBs from a new proj and re-upload to every per-frame
// buffer. Caller must ensure the GPU is idle (swapchain recreate guarantees this).
int uploadClusterAABBs(LogicalDevice* ld, SSBOBufferManager* ssbos, CommandPools* commandPool,
                       mat4 proj, float nearZ, float farZ);
/*uint32_t createOclCullingSSBOBuffer(PhysicalDevice* pd, LogicalDevice* ld, uint32_t ssbosLen, SSBOBufferManager*
   ssbos, CommandPools* commandPool, SCENE* scene);*/
