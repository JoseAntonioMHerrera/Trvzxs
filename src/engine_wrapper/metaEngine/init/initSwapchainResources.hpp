#pragma once

#include "initContext.hpp"

#include <SDL2/SDL.h>

// Phase 5 — initSwapchainResources
//
// Creates all per-swapchain resources. Structured in three stages:
//
//   1. Device locks (pd/ld) + validation (physicalDeviceTest, isDeviceValid).
//   2. Swapchain creation + dsm propagation to particle/light/depth pipelines
//      (all four pipelines share a single GraphicSwapchain*).
//   3. Per-frame attachment resources (MSAA color/depth, depth image, G-buffer,
//      normals buffer + sampler), legacy multi-subpass render pass (kept until
//      the render graph fully replaces it), swapchain image views, framebuffers,
//      command pools (graphics/transfer/compute) + cross-pipeline propagation,
//      graphics + compute command buffers.
//
// Inputs read from ctx (must be populated by Phase 6 — initDisplay):
//   dm, dispM, gdp, particlePipeline, lightPipeline, depthPrepassPipeline
//
// Outputs written back to ctx:
//   pd, ld, dsm
void metaInitSwapchainResources(MetaInitContext &ctx, SDL_Window *window);
