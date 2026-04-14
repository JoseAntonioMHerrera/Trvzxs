#pragma once

#include "initContext.hpp"

#include <SDL2/SDL.h>

// Forward-decl rather than including scene.hpp — keeps the wrapper layer's
// C++ Scene class out of the include graph for all downstream TUs that pull
// in metaEngine.hpp.
class Scene;

// Phase 6 — initDisplay
//
// Allocates the DisplayManager, the 4 graphic pipeline structs (models,
// particles, lights, depth prepass), their per-pipeline compute pipeline
// structs, DeviceBufferManager slots, CommandPools slots, and the shared
// GraphicSwapchain. Wires scene pointers and the starting MAX_FRAMES_IN_FLIGHT
// frame counts. Calls createSDLSurface + initDeviceManager.
//
// Inputs read from ctx (must be populated by Phase 7 — initInstance):
//   dm
//
// Outputs written back to ctx:
//   dispM, gdp, particlePipeline, lightPipeline, depthPrepassPipeline
void metaInitDisplay(MetaInitContext &ctx, Scene *scene, SDL_Window *window);
