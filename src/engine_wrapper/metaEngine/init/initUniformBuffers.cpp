#include "initUniformBuffers.hpp"

#include <cstdlib>

extern "C"
{
#include "uniformBuffer.h"
}

void metaInitUniformBuffers(MetaInitContext &ctx)
{
    PhysicalDevice           *pd                         = ctx.pd;
    LogicalDevice            *ld                         = ctx.ld;
    GraphicPipelineToSurface *gdp                        = ctx.gdp;
    GraphicPipelineToSurface *particleGraphicPipeline    = ctx.particlePipeline;
    GraphicPipelineToSurface *lightVolumeGraphicPipeline = ctx.lightPipeline;

    gdp->deviceBuffer->pUbo =
        (UniformBufferManager *)calloc(1, sizeof(*gdp->deviceBuffer->pUbo));
    particleGraphicPipeline->computeP->deviceBuffer->pUbo =
        (UniformBufferManager *)calloc(1, sizeof(*gdp->deviceBuffer->pUbo));
    lightVolumeGraphicPipeline->deviceBuffer->pUbo =
        (UniformBufferManager *)calloc(1,
            sizeof(*lightVolumeGraphicPipeline->deviceBuffer->pUbo));

    createModelInstanceTransformationsUniformBuffersMapped(
        pd, ld, ld->frameSync.maxFramesInFlight, gdp->deviceBuffer->pUbo,
        gdp->scene->modelsManager.modelInstanceCapacity);
    createLightInstanceTransformationsUniformBuffersMapped(
        pd, ld, ld->frameSync.maxFramesInFlight,
        lightVolumeGraphicPipeline->deviceBuffer->pUbo,
        lightVolumeGraphicPipeline->scene->lightManager
            .lightInstanceCapacity);

    // Per-frame globals UBO (binding 9) — shared across all pipelines
    gdp->pGlobalsUbo = (UniformBufferManager *)calloc(1, sizeof(*gdp->pGlobalsUbo));
    createFrameGlobalsUniformBuffersMapped(pd, ld, ld->frameSync.maxFramesInFlight, gdp->pGlobalsUbo);

    createDeltaTimeUniformBuffers(
        pd, ld, ld->frameSync.maxFramesInFlight,
        particleGraphicPipeline->computeP->deviceBuffer->pUbo);
}
