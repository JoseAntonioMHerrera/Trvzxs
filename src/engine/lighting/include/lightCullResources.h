#pragma once

#include "bufferTypes.h"
#include "bindlessTypes.h"

// Clustered forward+ Phase 2 — GPU resources produced by lightCull.comp and
// consumed by modelPass.frag.
//
// All three buffers are per-frame-in-flight: each has maxFrames copies, cleared
// and rewritten every frame by the compute pass.
//
//   lightIndexList   — flat, cluster-major array of light indices. Logical
//                      layout: [c0.l0, c0.l1, ..., c0.ln, c1.l0, ...]. Written
//                      via atomicAdd on globalIndexCount.
//                      Size per frame = CLUSTER_COUNT × MAX_LIGHTS_PER_CLUSTER × 4 B.
//
//   lightGrid        — per-cluster { offset, count } into lightIndexList.
//                      Size per frame = CLUSTER_COUNT × 8 B (two uint32).
//
//   globalIndexCount — monotonic counter written by atomicAdd, used by the
//                      compute pass to allocate slots in lightIndexList.
//                      Zeroed by vkCmdFillBuffer at the start of each frame's
//                      lightCull pass. Size = 4 B.
//
// Each SSBOBufferManager owns the maxFrames ring for its buffer type. Bindless
// handles are registered once at init; buffers are stable (no recreation per
// frame, only per swapchain recreate if grid dims change — they don't today).
typedef struct LightCullResources
{
    SSBOBufferManager lightIndexList;    // per-frame ring
    SSBOBufferManager lightGrid;         // per-frame ring
    SSBOBufferManager globalIndexCount;  // per-frame ring (one uint32 each)

    BindlessHandle*   lightIndexListHandles;   // [maxFrames]
    BindlessHandle*   lightGridHandles;        // [maxFrames]
    BindlessHandle*   globalIndexCountHandles; // [maxFrames]
} LightCullResources;
