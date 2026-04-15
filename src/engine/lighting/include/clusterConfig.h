#pragma once

// Clustered Forward+ lighting grid configuration.
//
// Grid subdivides the view frustum into CLUSTER_X * CLUSTER_Y * CLUSTER_Z
// cells. X/Y are uniform in NDC; Z is logarithmic in view space, so near
// slices are thin and far slices are thick (Tiago Sousa's formula).
//
// Sizing rationale (starting values):
//   16 x 9 x 24 = 3456 clusters. Matches 16:9 screen aspect, 24 Z-slices
//   covers typical near/far ratios (0.1..30000) with reasonable slice
//   thickness growth.
//
// MAX_LIGHTS_PER_CLUSTER bounds the per-cluster light list. Total flat
// index list size = CLUSTER_COUNT * MAX_LIGHTS_PER_CLUSTER indices (uint32).
// 3456 * 64 = 221184 uint32 = 864 KiB. Fits comfortably.

#define CLUSTER_X 16
#define CLUSTER_Y 9
#define CLUSTER_Z 24
#define CLUSTER_COUNT (CLUSTER_X * CLUSTER_Y * CLUSTER_Z)

#define MAX_LIGHTS_PER_CLUSTER 64
#define CLUSTER_LIGHT_INDEX_COUNT (CLUSTER_COUNT * MAX_LIGHTS_PER_CLUSTER)
