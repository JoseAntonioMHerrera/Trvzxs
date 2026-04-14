#pragma once

#include "initContext.hpp"

// Phase 4a — initTextures
//
// Creates the model texture image, image view, and (mipmap-aware) sampler.
// Currently a single texture attached to the models pipeline (gdp->texture).
//
// Bindless (A11) hook: this is where textures will be registered into the
// global bindless descriptor table. Splitting this out from initBuffers means
// A11 only has to reorder the orchestration call, not surgically split a
// larger function.
void metaInitTextures(MetaInitContext &ctx);
