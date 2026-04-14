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
        pd, ld, gdp->maxFrames, gdp->deviceBuffer->pUbo,
        gdp->scene->modelsManager.modelInstanceCapacity);
    createLightInstanceTransformationsUniformBuffersMapped(
        pd, ld, lightVolumeGraphicPipeline->maxFrames,
        lightVolumeGraphicPipeline->deviceBuffer->pUbo,
        lightVolumeGraphicPipeline->scene->lightManager
            .lightInstanceCapacity);

    // Per-frame globals UBO (binding 9) — shared across all pipelines
    gdp->pGlobalsUbo = (UniformBufferManager *)calloc(1, sizeof(*gdp->pGlobalsUbo));
    createFrameGlobalsUniformBuffersMapped(pd, ld, gdp->maxFrames, gdp->pGlobalsUbo);

    createDeltaTimeUniformBuffers(
        pd, ld, gdp->maxFrames,
        particleGraphicPipeline->computeP->deviceBuffer->pUbo);
}
