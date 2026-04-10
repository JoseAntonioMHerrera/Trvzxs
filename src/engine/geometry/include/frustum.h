#pragma once

#include "cglm.h"
#include <stdbool.h>

typedef struct AABB
{
    vec3 min;
    vec3 max;
} AABB;

typedef struct FrustumPlanes
{
    vec4 planes[6]; // left, right, bottom, top, near, far — each (a,b,c,d) normalized
} FrustumPlanes;

// Extract 6 frustum planes from a view-projection matrix (Gribb/Hartmann).
// Vulkan [0,1] depth range: near plane = row2, not row2+row3.
void frustumExtractPlanes(mat4 viewProj, FrustumPlanes* out);

// Test a model-space AABB against frustum planes using Arvo transform + p-vertex test.
// Returns true if the AABB is (at least partially) inside the frustum.
bool frustumTestAABB(const FrustumPlanes* frustum, const AABB* localAABB, mat4 worldMatrix);

// Compute an AABB from an array of GEOM_VERTEX (scans .point.x/y/z for min/max).
// vertexCount must be > 0.
struct GEOM_VERTEX;
void computeAABBFromVertices(const struct GEOM_VERTEX* vertices, uint32_t vertexCount, AABB* out);

// Cull all model and light instances against the frustum.
// Sets TRANSFORM_PASS_DISABLED on transforms whose AABB is fully outside.
// Must run after resolveTransforms() and before UBO fills.
typedef struct SCENE SCENE;
void frustumCullScene(SCENE* scene, const FrustumPlanes* frustum);
