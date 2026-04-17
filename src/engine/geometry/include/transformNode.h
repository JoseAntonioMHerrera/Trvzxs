#pragma once

#include <stdint.h>
#include "cglm.h"
#include "frustum.h"
#include "sceneEntity.h"

// Sentinel: no parent in the transform hierarchy
#define TRANSFORM_NO_PARENT UINT32_MAX

// TransformNode lifecycle flags (same semantics as instance lifecycle)
#define TRANSFORM_ALIVE     1
#define TRANSFORM_DISABLED  0
#define TRANSFORM_DESTROYED 2

// passFlags bits.
//
// DISABLED is per-frame: cleared at the start of each resolveTransforms
// iteration and re-set by resolve (lifecycle/cascade), cull (frustum miss),
// and instance-level lifecycle checks.
//
// DIRTY is sticky across frames: set by mutators and by transformInitHot,
// propagated down the subtree by resolveTransforms during the preorder walk,
// and cleared by a second sequential sweep at the end of resolve. A node
// whose DIRTY bit is NOT set when resolve visits it keeps its cached
// worldMatrix from the previous frame instead of recomputing.
#define TRANSFORM_PASS_DISABLED      1
#define TRANSFORM_PASS_DIRTY         2
// Set by frustumCullScene (which now runs before resolveTransforms) when a
// node's last-frame worldMatrix places its AABB outside the current frustum.
// Sticky across frames: cull re-decides each frame and clears the bit on
// visible nodes; resolve clears it on nodes it actually resolved. The DIRTY
// bit overrides this — cull never sets FRUSTUM_CULLED on a dirty node because
// its worldMatrix is stale by definition.
#define TRANSFORM_PASS_FRUSTUM_CULLED 4

// Resolve's skip mask: nodes carrying any of these bits keep their cached
// worldMatrix (or keep their DIRTY bit set, in the FRUSTUM_CULLED case, so
// they resolve the next frame they become visible).
#define TRANSFORM_RESOLVE_SKIP_MASK \
    (TRANSFORM_PASS_DISABLED | TRANSFORM_PASS_FRUSTUM_CULLED)

// Forces resolveTransforms to ignore FRUSTUM_CULLED for exactly one frame
// after a camera cut / teleport / scene reload, so stale cached matrices
// can't cause one-frame ghost renders. Set by callers that invalidate view
// state; consumed and cleared by the next resolveTransforms.
extern int g_visibilityCacheInvalidated;

// Per-frame loop state for a transform. Walked every frame by resolve, cull,
// and UBO fill for lifecycle/cascade/dirty decisions. Kept deliberately small
// (12 B) and separated from worldMatrix so a sequential walk of the whole
// array fits in L2 at 20K+ entries instead of bleeding into L3/DRAM.
typedef struct TransformHot
{
    uint32_t parentIdx;   //  4 B — preorder index of parent, or TRANSFORM_NO_PARENT
    uint16_t lifecycle;   //  2 B — TRANSFORM_ALIVE / DISABLED / DESTROYED
    uint16_t passFlags;   //  2 B — per-frame cascade/cull disable + sticky DIRTY
    uint32_t skipCount;   //  4 B — size of this node's subtree (0 = leaf), populated
                          //         by the load-time DFS emit. Consumed by
                          //         resolve/cull for bulk-skip.
} TransformHot;

// World matrix, held in a parallel array and only touched when actually
// recomputed (resolve dirty branch) or consumed (cull frustum test, UBO fill).
// 64 B = exactly one cache line, so a cull miss reads one line, a UBO fill
// read reads one line, no fragmentation.
typedef struct TransformWorld
{
    mat4 worldMatrix;     // 64 B — produced by resolve, consumed by cull + UBO fill
} TransformWorld;

// Cold-path slice. Read by resolve's dirty branch to rebuild the world matrix
// from TRS, and by the light UBO fill to sample uniform scale. Kept to 48 B
// (TRS only): the local matrix used to live here too but was pure scratch —
// resolve is the sole consumer and now computes it into a stack temporary,
// which halves the dirty-branch cache footprint from 112 B → 48 B per entry.
typedef struct TransformCold
{
    vec3  position;    // 12 B + 4 pad
    float _pad1;
    versor rotation;   // 16 B (quaternion), 16 B aligned for SSE
    vec3  scale;       // 12 B + 4 pad
    float _pad2;
} TransformCold;

typedef struct TransformManager
{
    TransformHot*   pTransformsHot;    // [count] — 12 B loop state, walked every frame
    TransformWorld* pTransformsWorld;  // [count] — 64 B world matrix, lazy (resolve writes, cull/UBO fill read)
    TransformCold*  pTransformsCold;   // [count] — cold TRS + local matrix
    PreorderEntity* pPreorderEntities; // [count] — reverse index: preorder pos → backing instance
    AABB*           pLocalAABBs;       // [count] — per-entity local-space AABB (copied from type at load)
    uint32_t        count;             // total allocated slots
    uint32_t        liveCount;         // updated per frame by resolve pass
} TransformManager;

// --- Init helpers ---

// Initialize a single cold slot: zero position, identity rotation, unit scale.
void transformInitCold(TransformCold* cold);

// Initialize a single hot slot: no parent, alive, DIRTY bit set (so the first
// resolve computes the worldMatrix from fresh TRS).
void transformInitHot(TransformHot* hot);

// Initialize a single world slot to identity. Only needed for pre-resolve
// reads from tests — resolve overwrites it anyway on the first pass because
// the hot slot starts with DIRTY set.
void transformInitWorld(TransformWorld* world);

// Convenience: initialize hot + world + cold at index `idx`. Caller must
// ensure `idx < man->count` and that all three arrays are allocated.
void transformInit(TransformManager* man, uint32_t idx);

// --- TRS setters (operate on cold) ---

void transformSetPosition(TransformCold* cold, vec3 pos);
void transformSetRotation(TransformCold* cold, versor q);
void transformSetRotationEuler(TransformCold* cold, float pitchDeg, float yawDeg, float rollDeg);
void transformSetScale(TransformCold* cold, vec3 scale);
void transformSetScaleUniform(TransformCold* cold, float s);

// Incremental modification (cold)
void transformTranslate(TransformCold* cold, vec3 delta);
void transformRotateAxis(TransformCold* cold, float angleDeg, vec3 axis);

// Compose local matrix = T * R * S into the caller-provided mat4. No state
// lives in TransformCold for this — resolveTransforms passes a stack temp.
void transformBuildLocalMatrix(const TransformCold* cold, mat4 outLocal);

// --- Hierarchy (operates on the manager, touches hot) ---

// Set parent-child relationship. Validates parent index < child index.
void transformSetParent(TransformManager* man, uint32_t childIdx, uint32_t parentIdx);

// Clear parent (make node a root).
void transformClearParent(TransformManager* man, uint32_t idx);

// Mark a transform's worldMatrix as stale. Next resolveTransforms will
// recompose a fresh worldMatrix from TRS and propagate DIRTY to every descendant. Idempotent. Required after any direct mutation
// to pTransformsCold[idx] that doesn't go through the instance-wrapper
// helpers in transform.c.
void transformMarkDirty(TransformManager* man, uint32_t idx);

// --- Per-frame resolve ---

// Single forward pass: builds local matrices, resolves hierarchy, cascade-disables.
// Updates liveCount.
void resolveTransforms(TransformManager* man);
