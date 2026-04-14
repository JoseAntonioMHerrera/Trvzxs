#pragma once

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720
#define MAX_FRAME_COUNT 400

// --- LOD selection thresholds (projected pixel size, vertical screen-axis) ---
// Picked at load time in src/engine/geometry/scene.c::fillSceneWithModelData.
// An instance's projected pixel size is `radius * (screenH/2) / (tan(FOV/2) * dist)`;
// the smallest LOD index whose minPixelSize is satisfied wins. lod[MODEL_LOD_COUNT-1]
// uses LOD_MIN_PX_COARSEST = 0 so selection is always total.
//
// Tuned against the viking_room (radius ≈ 1.1) at 720p + 45° FOV. For a radius-R asset
// at distance D, pixelSize ≈ R * 869 / D. Current values fire:
//   LOD 0 for dist ≤ ~48 units
//   LOD 1 for ~48 < dist ≤ ~128 units
//   LOD 2 everywhere else
// Larger-radius assets will want proportionally higher thresholds. Long term these
// should migrate into per-asset metadata; for now they're a single global knob.
#define LOD_MIN_PX_FULL 20.0f    // lod[0] — full-detail mesh
#define LOD_MIN_PX_MEDIUM 15.0f  // lod[1] — ~30% simplification
#define LOD_MIN_PX_COARSEST 0.0f // lod[MODEL_LOD_COUNT-1] — catch-all
