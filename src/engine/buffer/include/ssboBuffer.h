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

// Clustered forward+ Phase 2 output SSBO creators. Each allocates `ssbosLen`
// device-local VkBuffers sized for one frame-in-flight copy. No initial data
// upload — contents are written each frame by lightCull.comp (globalIndexCount
// is cleared via vkCmdFillBuffer at the start of the lightCull render graph pass).

// lightIndexList: flat cluster-major array of light indices.
// Per-frame size = CLUSTER_COUNT * MAX_LIGHTS_PER_CLUSTER * sizeof(uint32).
int createLightIndexListSSBOs(PhysicalDevice* pd, LogicalDevice* ld, uint32_t ssbosLen,
                              SSBOBufferManager* ssbos);

// lightGrid: per-cluster {offset, count}.
// Per-frame size = CLUSTER_COUNT * 2 * sizeof(uint32).
int createLightGridSSBOs(PhysicalDevice* pd, LogicalDevice* ld, uint32_t ssbosLen,
                         SSBOBufferManager* ssbos);

// globalIndexCount: single uint32 counter (atomic) per frame.
int createGlobalIndexCountSSBOs(PhysicalDevice* pd, LogicalDevice* ld, uint32_t ssbosLen,
                                SSBOBufferManager* ssbos);
/*uint32_t createOclCullingSSBOBuffer(PhysicalDevice* pd, LogicalDevice* ld, uint32_t ssbosLen, SSBOBufferManager*
   ssbos, CommandPools* commandPool, SCENE* scene);*/
