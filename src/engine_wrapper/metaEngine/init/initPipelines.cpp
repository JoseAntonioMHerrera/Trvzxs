#include "initPipelines.hpp"

extern "C"
{
#include "computePipeline.h"
#include "graphicsPipeline.h"
}

void metaInitPipelines(MetaInitContext &ctx, RenderGraph *renderGraph)
{
    DisplayManager           *dispM                       = ctx.dispM;
    LogicalDevice            *ld                          = ctx.ld;
    GraphicPipelineToSurface *gdp                         = ctx.gdp;
    GraphicPipelineToSurface *particleGraphicPipeline     = ctx.particlePipeline;
    GraphicPipelineToSurface *lightVolumeGraphicPipeline  = ctx.lightPipeline;
    GraphicPipelineToSurface *depthPrepassGraphicPipeline = ctx.depthPrepassPipeline;

    // Create graphics pipelines AFTER rgCompile so they use the render
    // graph's single-subpass render passes (subpass 0) instead of the
    // old multi-subpass render pass.

    // Depth prepass pipeline — uses the render graph's depth-only render pass
    depthPrepassGraphicPipeline->renderPass = &renderGraph->passes[ctx.pDepthPrepass].renderPass;
    depthPrepassGraphicPipeline->dsl = gdp->dsl;
    depthPrepassGraphicPipeline->descriptorSet = gdp->descriptorSet;
    createDepthPrepassGraphicsPipeline(depthPrepassGraphicPipeline, dispM, &ld->device);

    gdp->renderPass = &renderGraph->passes[ctx.pModels].renderPass;
    createModelsGraphicPipeline(gdp, dispM, &ld->device);

    lightVolumeGraphicPipeline->renderPass = &renderGraph->passes[ctx.pLights].renderPass;
    createLightVolumeGraphicPipeline(lightVolumeGraphicPipeline, dispM, &ld->device);

    particleGraphicPipeline->renderPass = &renderGraph->passes[ctx.pParticles].renderPass;
    createParticleGraphicPipeline(particleGraphicPipeline, dispM, &ld->device);

    // Compute pipeline needs particle pipelineLayout (created above)
    particleGraphicPipeline->computeP->pipelineLayout =
        particleGraphicPipeline->pipelineLayout;
    createParticleComputePipeline(particleGraphicPipeline->computeP,
                                  ld->device);
}
