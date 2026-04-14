#pragma once

#include "initContext.hpp"

// Phase 4b — initUniformBuffers
//
// Allocates UniformBufferManager slots and creates the host-mapped UBOs:
//   - Model instance transformations (gdp pipeline, sized by modelInstanceCapacity)
//   - Light instance transformations (light pipeline, sized by lightInstanceCapacity)
//   - Frame globals (shared across all pipelines, binding 9)
//   - Delta time (particle compute pipeline)
void metaInitUniformBuffers(MetaInitContext &ctx);
