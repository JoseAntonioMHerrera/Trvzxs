#include "initSwapchainResources.hpp"

#include <stdexcept>

extern "C"
{
#include "commandBuffer.h"
#include "commandPool.h"
#include "image.h"
#include "main_vulkan.h"
#include "swapchainManager.h"
}

void metaInitSwapchainResources(MetaInitContext &ctx, SDL_Window *window)
{
    DeviceManager            *dm                         = ctx.dm;
    DisplayManager           *dispM                      = ctx.dispM;
    GraphicPipelineToSurface *gdp                        = ctx.gdp;
    GraphicPipelineToSurface *particleGraphicPipeline    = ctx.particlePipeline;
    GraphicPipelineToSurface *lightVolumeGraphicPipeline = ctx.lightPipeline;
    GraphicPipelineToSurface *depthPrepassGraphicPipeline = ctx.depthPrepassPipeline;

    // Stage 1: lock physical + logical device handles, run validation
    PhysicalDevice *pd = &dm->pDevice[dm->gpuIndex];
    LogicalDevice  *ld = dm->lDevice[dm->deviceIndex];

    physicalDeviceTest(pd->gpu);
    if (VK_FALSE == isDeviceValid(ld->device))
    {
        throw std::runtime_error("isDeviceValid returns VK_FALSE");
    }

    // Stage 2: swapchain + shared dsm propagation
    createSwapChain(pd, ld, dispM, gdp, window);
    GraphicSwapchain *dsm = gdp->swapchain;
    particleGraphicPipeline->swapchain    = dsm;
    lightVolumeGraphicPipeline->swapchain = dsm;
    depthPrepassGraphicPipeline->swapchain = dsm;

    gdp->maxFrames                       = dsm->scImagesLen;
    particleGraphicPipeline->maxFrames   = dsm->scImagesLen;
    lightVolumeGraphicPipeline->maxFrames = dsm->scImagesLen;
    depthPrepassGraphicPipeline->maxFrames = dsm->scImagesLen;

    // Stage 3: per-frame attachments, render pass, image views, framebuffers,
    // command pools, command buffers.

    createMsaaColorResources(ld, pd, dsm);
    createMsaaDepthResources(ld, pd, dsm); // multisampled geometry pipeline
    createDepthImageResources(pd, ld, dispM->graphicPipeline->swapchain); // non-MSAA
    createGeometryBufferResources(pd, ld, dispM->graphicPipeline->swapchain);
    createNormalsBufferResources(pd, ld, dispM->graphicPipeline->swapchain);
    createGBufferSampler(ld->device, dispM->graphicPipeline->swapchain);

    // Legacy multi-subpass render pass — kept until the render graph's
    // single-subpass render passes fully replace it. Pipeline creation is
    // deferred to Phase 2 (initPipelines, post-rgCompile) using the graph's
    // render passes; this call populates gdp->renderPass only as a placeholder.
    createRenderPass(pd, dispM->graphicPipeline[0].swapchain, ld->device, gdp);
    particleGraphicPipeline->renderPass    = gdp->renderPass;
    lightVolumeGraphicPipeline->renderPass = gdp->renderPass;

    dsm->scImageViewLen = dsm->scImagesLen;
    for (uint32_t i = 0; i < dsm->scImagesLen; i++)
    {
        dsm->p_scImageView[i] = createImageView(
            dsm, ld->device, dsm->currentSurfaceFormat->format,
            dsm->p_scImages[i],
            1, VK_IMAGE_ASPECT_COLOR_BIT);
    }

    createFrameBuffers(dsm, gdp->renderPass, &ld->device);

    createGraphicsCommandPool(pd, ld->device, gdp);
    createTransferQueueCommandPool(pd, ld->device, gdp);
    createComputeQueueCommandPool(pd, ld->device, gdp);

    // All pipelines share the same command pool for now (TODO: per-pipeline
    // pools when async transfer / compute queues diverge).
    particleGraphicPipeline->commandPool    = gdp->commandPool;
    lightVolumeGraphicPipeline->commandPool = gdp->commandPool;
    depthPrepassGraphicPipeline->commandPool = gdp->commandPool;
    for (uint32_t compIndex = 0; compIndex < particleGraphicPipeline->computePipelineCount; compIndex++)
    {
        particleGraphicPipeline->computeP[compIndex].commandPool = particleGraphicPipeline->commandPool;
    }
    for (uint32_t compIndex = 0; compIndex < depthPrepassGraphicPipeline->computePipelineCount; compIndex++)
    {
        depthPrepassGraphicPipeline->computeP[compIndex].commandPool = depthPrepassGraphicPipeline->commandPool;
    }

    createCommandBuffer(gdp, &ld->device, gdp->maxFrames);
    createCommandBuffer(lightVolumeGraphicPipeline, &ld->device,
                        lightVolumeGraphicPipeline->maxFrames);
    createCommandBufferWithCompute(particleGraphicPipeline, &ld->device,
                                   gdp->maxFrames);

    // Publish outputs
    ctx.pd  = pd;
    ctx.ld  = ld;
    ctx.dsm = dsm;
}
