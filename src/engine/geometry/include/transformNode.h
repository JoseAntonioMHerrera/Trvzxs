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

// Per-frame pass flag
#define TRANSFORM_PASS_DISABLED 1

// Hot-path slice of a transform. Touched every frame by cull, resolve, and
// UBO fill. sizeof = 80 B after mat4 alignment padding (fits skipCount for
// free within the existing pad).
typedef struct TransformHot
{
    mat4     worldMatrix; // 64 B — produced by resolve, consumed by cull + UBO fill
    uint32_t parentIdx;   //  4 B — preorder index of parent, or TRANSFORM_NO_PARENT
    uint16_t lifecycle;   //  2 B — TRANSFORM_ALIVE / DISABLED / DESTROYED
    uint16_t passFlags;   //  2 B — per-frame cascade / cull disable
    uint32_t skipCount;   //  4 B — size of this node's subtree (0 = leaf), populated
                          //         by the load-time DFS emit. Consumed by
                          //         resolve/cull in Phase 3.
} TransformHot;

// Cold-path slice. Read by resolve to rebuild local + world matrices, and by
// the light UBO fill to sample uniform scale. Otherwise untouched per frame.
typedef struct TransformCold
{
    vec3  position;    // 12 B + 4 pad
    float _pad1;
    versor rotation;   // 16 B (quaternion)
    vec3  scale;       // 12 B + 4 pad
    float _pad2;
    mat4  localMatrix; // 64 B — T * R * S, rebuilt each frame by resolve
} TransformCold;

typedef struct TransformManager
{
    TransformHot*   pTransformsHot;    // [count] — hot working set
    TransformCold*  pTransformsCold;   // [count] — cold TRS + local matrix
    PreorderEntity* pPreorderEntities; // [count] — reverse index: preorder pos → backing instance
    AABB*           pLocalAABBs;       // [count] — per-entity local-space AABB (copied from type at load)
    uint32_t        count;             // total allocated slots
    uint32_t        liveCount;         // updated per frame by resolve pass
} TransformManager;

// --- Init helpers ---

// Initialize a single cold slot: zero TRS, identity localMatrix.
void transformInitCold(TransformCold* cold);

// Initialize a single hot slot: identity worldMatrix, no parent, alive, no flags.
void transformInitHot(TransformHot* hot);

// Convenience: initialize both arrays at index `idx`. Caller must ensure
// `idx < man->count` and that both arrays are allocated.
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

// Rebuild localMatrix from TRS (called by resolveTransforms, or manually)
void transformBuildLocalMatrix(TransformCold* cold);

// --- Hierarchy (operates on the manager, touches hot) ---

// Set parent-child relationship. Validates parent index < child index.
void transformSetParent(TransformManager* man, uint32_t childIdx, uint32_t parentIdx);

// Clear parent (make node a root).
void transformClearParent(TransformManager* man, uint32_t idx);

// --- Per-frame resolve ---

// Single forward pass: builds local matrices, resolves hierarchy, cascade-disables.
// Updates liveCount.
void resolveTransforms(TransformManager* man);
