#pragma once

#include "initContext.hpp"

// Phase 3 — initDescriptors
//
// Creates the descriptor pool, descriptor set layout, and descriptor set used
// across all graphics pipelines. Propagates the resulting dsl/pool/set into
// the particle and light pipelines.
//
// Currently a single monolithic descriptor set with 9 bindings is shared by
// all pipelines and the compute pipeline (issue I9 in the roadmap). A11
// (bindless) will rewrite this whole module in place — the global set + pool
// shape is the right seam for that retrofit.
//
// MUST run after textures, UBOs, and SSBOs are created (createDescriptorSet
// binds them).
void metaInitDescriptors(MetaInitContext &ctx);
