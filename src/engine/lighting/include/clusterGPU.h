#pragma once

#include "deviceManagmentTypes.h"
#include "geometryPipelineTypes.h"
#include "swapchainManager.h"

// Clustered forward+ Phase 1: rebuild cluster AABBs after a swapchain recreate.
//
// Call this right after recreateSwapChain(). It:
//   1. Reconstructs the projection matrix from the new swapchain extent
//      (mirrors updateFrameGlobals — 45° FOV, near=0.1, far=30000).
//   2. Runs buildClusterAABBs on the CPU.
//   3. Stages + uploads the new grid to every per-frame cluster AABB SSBO.
//
// recreateSwapChain already waits for device idle before returning, so
// overwriting the SSBO contents here is safe.
//
// No-op if `lightPipeline->computeP` is NULL or cluster SSBOs weren't allocated
// (e.g. headless tests).
#ifdef __cplusplus
extern "C" {
#endif

void rebuildClusterAABBsOnResize(LogicalDevice *ld,
                                 GraphicPipelineToSurface *lightPipeline,
                                 GraphicSwapchain *dsm);

#ifdef __cplusplus
}
#endif
