#include "initDescriptors.hpp"

extern "C"
{
#include "descriptor.h"
}

void metaInitDescriptors(MetaInitContext &ctx)
{
    LogicalDevice            *ld                          = ctx.ld;
    GraphicPipelineToSurface *gdp                         = ctx.gdp;
    GraphicPipelineToSurface *particleGraphicPipeline     = ctx.particlePipeline;
    GraphicPipelineToSurface *lightVolumeGraphicPipeline  = ctx.lightPipeline;

    createDescriptorPool(ld->frameSync.maxFramesInFlight, ld->device, gdp);
    createDescriptorSetLayout(ld, gdp, ld->frameSync.maxFramesInFlight);
    createDescriptorSet(ld->frameSync.maxFramesInFlight, ld->device, gdp,
                        particleGraphicPipeline,
                        lightVolumeGraphicPipeline, gdp->texture);
    particleGraphicPipeline->dsl            = gdp->dsl;
    particleGraphicPipeline->descriptorPool = gdp->descriptorPool;
    particleGraphicPipeline->descriptorSet  = gdp->descriptorSet;
    lightVolumeGraphicPipeline->descriptorSet  = gdp->descriptorSet;
    lightVolumeGraphicPipeline->descriptorPool = gdp->descriptorPool;
    lightVolumeGraphicPipeline->dsl            = gdp->dsl;
}
