#pragma once

#include "initContext.hpp"

extern "C"
{
#include "renderGraph.h"
}

// Phase 2 — initPipelines
//
// Post-rgCompile pipeline creation. Wires each pipeline's renderPass to the
// render-graph's single-subpass render pass (created during rgCompile), then
// builds the VkPipeline + VkPipelineLayout for depth prepass, models, lights,
// particles, and the particle compute shader.
//
// MUST run after metaInitRenderGraph (consumes ctx.pDepthPrepass / pModels /
// pLights / pParticles).
void metaInitPipelines(MetaInitContext &ctx, RenderGraph *renderGraph);
