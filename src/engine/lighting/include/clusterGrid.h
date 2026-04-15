#pragma once

#include <cglm/cglm.h>
#include "clusterConfig.h"

// View-space axis-aligned bounding box for a single cluster.
// Stored as two vec4s so the memory layout matches the SSBO the compute
// light-culling pass will read (std430, 16-byte aligned per element).
typedef struct ClusterAABB
{
    vec4 minView; // xyz = min corner in view space; w unused (padding)
    vec4 maxView; // xyz = max corner in view space; w unused
} ClusterAABB;

// Build the full cluster AABB grid in view space.
//
// Inputs:
//   proj       — projection matrix (left-handed, Vulkan [0,1] depth; matches
//                glm_perspective used in updateFrameGlobals).
//   nearZ/farZ — positive view-space Z bounds of the frustum.
// Output:
//   out[CLUSTER_COUNT] — caller-provided; filled with one AABB per cluster.
//
// Cluster index layout: idx = x + y * CLUSTER_X + z * CLUSTER_X * CLUSTER_Y.
//
// This function is pure and CPU-side. Call once at startup and again on
// swapchain resize / FOV change (anything that alters proj/near/far).
void buildClusterAABBs(mat4 proj, float nearZ, float farZ, ClusterAABB out[CLUSTER_COUNT]);

// Convert a view-space Z (positive, forward) to a cluster Z-slice index
// using the logarithmic distribution. Exposed for tests and for matching
// the formula against the fragment shader's slice computation.
int clusterSliceFromViewZ(float viewZ, float nearZ, float farZ);
