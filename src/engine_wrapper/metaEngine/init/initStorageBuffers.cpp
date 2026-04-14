#include "initStorageBuffers.hpp"

#include <cstdlib>

extern "C"
{
#include "ssboBuffer.h"
}

void metaInitStorageBuffers(MetaInitContext &ctx)
{
    PhysicalDevice           *pd                         = ctx.pd;
    LogicalDevice            *ld                         = ctx.ld;
    GraphicPipelineToSurface *gdp                        = ctx.gdp;
    GraphicPipelineToSurface *particleGraphicPipeline    = ctx.particlePipeline;
    GraphicPipelineToSurface *lightVolumeGraphicPipeline = ctx.lightPipeline;

    // Particle SSBO (owned by the particle compute pipeline's deviceBuffer)
    particleGraphicPipeline->computeP->deviceBuffer->ssbos =
        (SSBOBufferManager *)malloc(sizeof(*gdp->deviceBuffer->ssbos));
    createParticleSSBOBuffer(
        pd, ld, particleGraphicPipeline->maxFrames,
        particleGraphicPipeline->computeP->deviceBuffer->ssbos,
        particleGraphicPipeline->computeP->commandPool,
        particleGraphicPipeline->scene);

    // Light pass-1 readback SSBO
    lightVolumeGraphicPipeline->deviceBuffer->ssbos =
        (SSBOBufferManager *)malloc(sizeof(*lightVolumeGraphicPipeline->deviceBuffer->ssbos));
    createReadbackLightPass1SSBOBuffers(
        pd, ld, lightVolumeGraphicPipeline->maxFrames,
        lightVolumeGraphicPipeline->deviceBuffer->ssbos,
        lightVolumeGraphicPipeline->commandPool);
}
