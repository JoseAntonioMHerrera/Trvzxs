#pragma once
#include <SDL2/SDL.h>
#include <stddef.h>
#include "deviceManagmentTypes.h"
#include "uniformBuffer.h"
#include "swapchainManager.h"
#include "graphicsPipeline.h"
#define MAX_FRAMES_IN_FLIGHT 2

typedef struct RenderGraph RenderGraph;
void  drawFrame(PhysicalDevice *pd, LogicalDevice *ld, DisplayManager *dm, GraphicPipelineToSurface *gdp,
GraphicPipelineToSurface *particleGraphics, GraphicPipelineToSurface* lightGraphics, VkDevice device, SDL_Window *window, float deltaTime, Camera3D camera, RenderGraph* rg);
void cleanupSyncObjects(VkDevice device, GraphicPipelineToSurface* gdp);

void createSDLSurface(DisplayManager *dm, VkInstance* instance, SDL_Window* window);

// --- Benchmark per-scenario stats (DEBUG builds only) ---
// Begin a new scenario window: flushes the previous scenario's collected
// samples as a min/median/mean/p95/max stats block, then resets the buffer.
// End: forces a final flush at the end of the run. Both are no-ops in
// non-DEBUG builds. Samples are per-frame resolveTransforms, frustumCull
// and totalCpu timings, captured automatically from the frame loop.
void displayBenchBeginScenario(const char* name);
void displayBenchEndScenario(void);

// Record the current frame's LOD bucket distribution into the active
// scenario's accumulator. Sums lodLiveInstanceCount[] across all alive model
// types; the scenario report prints absolute totals and percentages. Call
// once per frame after updateModelUniformBuffersDirect. No-op in non-DEBUG.
struct SCENE;
void displayBenchRecordLodSample(const struct SCENE* scene);
