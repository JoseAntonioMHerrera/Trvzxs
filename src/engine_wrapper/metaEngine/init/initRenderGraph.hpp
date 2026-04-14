#pragma once

#include "initContext.hpp"

extern "C"
{
#include "passData.h"
#include "renderGraph.h"
}

// Phase 1 — initRenderGraph
//
// Builds the engine's render graph: registers external resources (color, depth,
// G-buffer, normals, particle SSBOs), declares 5 passes (depth prepass, models,
// lights, particles, particle compute) with their input/output dependencies and
// callbacks, then compiles the graph (topological sort + barrier generation +
// render pass creation).
//
// Writes pass indices into `ctx` (consumed by Phase 2 — initPipelines).
// Pass-data structs are populated via the provided pointers; their lifetime is
// owned by the caller (MetaEngine members).
void metaInitRenderGraph(MetaInitContext   &ctx,
                         RenderGraph       *renderGraph,
                         DepthPrepassData  *depthPrepassData,
                         ModelPassData     *modelPassData,
                         LightPassData     *lightPassData,
                         ParticlePassData  *particlePassData,
                         ComputePassData   *computePassData);
