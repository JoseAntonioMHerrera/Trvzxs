#include "initPipelines.hpp"

extern "C"
{
#include "computePipeline.h"
#include "graphicsPipeline.h"
#include "bindlessPushConstants.h"
}

#include <cstdlib>

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
    // depthPrepassGraphicPipeline->dsl = gdp->dsl;               // DORMANT A11
    // depthPrepassGraphicPipeline->descriptorSet = gdp->descriptorSet; // DORMANT A11
    createDepthPrepassGraphicsPipeline(depthPrepassGraphicPipeline, dispM, &ld->device);

    gdp->renderPass = &renderGraph->passes[ctx.pModels].renderPass;
    createModelsGraphicPipeline(gdp, dispM, &ld->device);

    lightVolumeGraphicPipeline->renderPass = &renderGraph->passes[ctx.pLights].renderPass;
    createLightVolumeGraphicPipeline(lightVolumeGraphicPipeline, dispM, &ld->device);

    particleGraphicPipeline->renderPass = &renderGraph->passes[ctx.pParticles].renderPass;
    createParticleGraphicPipeline(particleGraphicPipeline, dispM, &ld->device);

    // A11 — compute pipeline gets its own dedicated layout: bindless set 0 +
    // particleComputePass set 1 (delta time UBO) + COMPUTE-stage push
    // constants. Cannot share with graphics because the push-constant stage
    // mask and set-1 layouts differ.
    {
        ComputePipeline *cp = particleGraphicPipeline->computeP;
        VkDescriptorSetLayout computeSetLayouts[2] = {
            cp->bindlessGlobalLayout,
            cp->bindlessPerPassLayout,
        };
        VkPushConstantRange computePushRange = {
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
            .offset     = 0,
            .size       = sizeof(BindlessPushConstants),
        };
        VkPipelineLayoutCreateInfo computeLayoutInfo = {
            .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .setLayoutCount         = 2,
            .pSetLayouts            = computeSetLayouts,
            .pushConstantRangeCount = 1,
            .pPushConstantRanges    = &computePushRange,
        };
        cp->pipelineLayout = (VkPipelineLayout *)malloc(sizeof(*cp->pipelineLayout));
        if (vkCreatePipelineLayout(ld->device, &computeLayoutInfo, nullptr, cp->pipelineLayout) != VK_SUCCESS)
        {
            std::exit(1);
        }
    }
    createParticleComputePipeline(particleGraphicPipeline->computeP,
                                  ld->device);
}
