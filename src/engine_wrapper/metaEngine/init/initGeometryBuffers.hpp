#pragma once

#include "initContext.hpp"

// Phase 4d — initGeometryBuffers
//
// Creates all per-type packed geometry buffers (vertex + index), sync objects,
// and the Vulkan debug callback. Also zeros dispM->currentFrame.
//
// Buffers:
//   - Particle vertex + index
//   - Model vertex + index (packed across all Model3D types)
//   - Light volume vertex + index (packed across all LightVolume types;
//     index buffer creation skipped if packedLightIndex is empty)
//
// Sync:
//   - createSyncObjects for models (gdp) and particle pipelines
//   - createFrameSync on the logical device (per-frame semaphores/fences)
//
// MUST run after command pools are created (uses commandPool for staging
// transfers) and before render-graph compile (graph uses SSBO handles but
// not vertex/index; geometry buffers are safe to create at any point after
// the SSBOs land).
void metaInitGeometryBuffers(MetaInitContext &ctx);
