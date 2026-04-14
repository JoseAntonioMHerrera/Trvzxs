#pragma once

#include "initContext.hpp"

// A11 Phase 3b — register existing resources into the bindless system and
// allocate per-pass descriptor sets for pipelines that have been migrated.
//
// Runs after all resources exist (initTextures, initUniformBuffers,
// initStorageBuffers, initGeometryBuffers) and before pipeline layout
// creation (initPipelines), which reads the layout handles this step
// plants on each GraphicPipelineToSurface.
//
// Current migration scope: model pipeline + depth prepass pipeline.
// Other pipelines (light, particle graphics, particle compute) keep the
// dormant explicit-descriptor path until their respective cutovers.
void metaInitBindlessRegistration(MetaInitContext &ctx);
