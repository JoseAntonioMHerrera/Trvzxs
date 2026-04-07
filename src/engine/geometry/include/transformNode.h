#pragma once

#include <stdint.h>
#include "cglm.h"

// Sentinel: no parent in the transform hierarchy
#define TRANSFORM_NO_PARENT UINT32_MAX

// TransformNode lifecycle flags (same semantics as instance lifecycle)
#define TRANSFORM_ALIVE     1
#define TRANSFORM_DISABLED  0
#define TRANSFORM_DESTROYED 2

// Per-frame pass flag
#define TRANSFORM_PASS_DISABLED 1

typedef struct TransformNode
{
    vec3     position;
    float    _pad1;
    versor   rotation; // quaternion
    vec3     scale;
    float    _pad2;
    mat4     localMatrix;  // T * R * S, rebuilt each frame
    mat4     worldMatrix;  // localMatrix * parent.worldMatrix (final result)
    uint32_t parentIdx;    // index into TransformManager.pTransforms, or TRANSFORM_NO_PARENT
    uint16_t lifecycle;    // TRANSFORM_ALIVE / DISABLED / DESTROYED
    uint16_t passFlags;    // per-frame cascade-disable
} TransformNode;

typedef struct TransformManager
{
    TransformNode* pTransforms; // single dense allocation
    uint32_t       count;       // total allocated slots
    uint32_t       liveCount;   // updated per frame by resolve pass
} TransformManager;

// --- Init / TRS setters (type-agnostic) ---

void transformInit(TransformNode* node);
void transformSetPosition(TransformNode* node, vec3 pos);
void transformSetRotation(TransformNode* node, versor q);
void transformSetRotationEuler(TransformNode* node, float pitchDeg, float yawDeg, float rollDeg);
void transformSetScale(TransformNode* node, vec3 scale);
void transformSetScaleUniform(TransformNode* node, float s);

// Incremental modification
void transformTranslate(TransformNode* node, vec3 delta);
void transformRotateAxis(TransformNode* node, float angleDeg, vec3 axis);

// Rebuild localMatrix from TRS (called by resolveTransforms, or manually)
void transformBuildLocalMatrix(TransformNode* node);

// --- Hierarchy ---

// Set parent-child relationship. Validates parent index < child index.
void transformSetParent(TransformManager* man, uint32_t childIdx, uint32_t parentIdx);

// Clear parent (make node a root).
void transformClearParent(TransformManager* man, uint32_t idx);

// --- Per-frame resolve ---

// Single forward pass: builds local matrices, resolves hierarchy, cascade-disables.
// Updates liveCount.
void resolveTransforms(TransformManager* man);
