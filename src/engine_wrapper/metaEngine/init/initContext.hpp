#pragma once

#include <cstdint>

extern "C"
{
#include "deviceManagment.h"
#include "geometryPipelineTypes.h"
#include "swapchainManager.h"
}

// MetaInitContext threads the working pointers through the I5 init pipeline.
// Each init phase populates a subset of these fields; later phases consume them.
// Lifetime is the scope of bulkInitVulkanWithScene only.
//
// Population order:
//   initInstance              -> dm
//   initDeviceFeatures        -> (no fields; configures dm prior to device creation)
//   initDisplay               -> dispM, gdp, particlePipeline, lightPipeline,
//                                depthPrepassPipeline, pd, ld
//   initSwapchainResources    -> dsm
//   initRenderGraph           -> pDepthPrepass, pModels, pLights, pParticles,
//                                pParticleCompute
//   initPipelines             -> consumes pass indices, creates VkPipelines
struct MetaInitContext
{
    // Phase 7 — initInstance
    DeviceManager *dm = nullptr;

    // Phase 6 — initDisplay
    DisplayManager           *dispM                = nullptr;
    GraphicPipelineToSurface *gdp                  = nullptr; // models pipeline (slot 0)
    GraphicPipelineToSurface *particlePipeline     = nullptr; // slot 1
    GraphicPipelineToSurface *lightPipeline        = nullptr; // slot 2
    GraphicPipelineToSurface *depthPrepassPipeline = nullptr; // slot 3
    PhysicalDevice           *pd                   = nullptr;
    LogicalDevice            *ld                   = nullptr;

    // Phase 5 — initSwapchainResources
    GraphicSwapchain *dsm = nullptr;

    // Phase 1 — initRenderGraph (consumed by Phase 2 — initPipelines)
    uint32_t pDepthPrepass   = 0;
    uint32_t pModels         = 0;
    uint32_t pLights         = 0;
    uint32_t pParticles      = 0;
    uint32_t pParticleCompute = 0;
};
