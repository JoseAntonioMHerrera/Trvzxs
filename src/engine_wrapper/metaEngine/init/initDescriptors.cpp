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

    createDescriptorPool(gdp->maxFrames, ld->device, gdp);
    createDescriptorSetLayout(ld, gdp, gdp->maxFrames);
    createDescriptorSet(gdp->maxFrames, ld->device, gdp,
                        particleGraphicPipeline,
                        lightVolumeGraphicPipeline, gdp->texture);
    particleGraphicPipeline->dsl            = gdp->dsl;
    particleGraphicPipeline->descriptorPool = gdp->descriptorPool;
    particleGraphicPipeline->descriptorSet  = gdp->descriptorSet;
    lightVolumeGraphicPipeline->descriptorSet  = gdp->descriptorSet;
    lightVolumeGraphicPipeline->descriptorPool = gdp->descriptorPool;
    lightVolumeGraphicPipeline->dsl            = gdp->dsl;
}
