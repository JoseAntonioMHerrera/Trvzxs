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

    // Stage 3: per-frame attachments, render pass, image views, framebuffers,
    // command pools, command buffers.

    createMsaaColorResources(ld, pd, dsm);
    createMsaaDepthResources(ld, pd, dsm); // multisampled geometry pipeline
    createDepthImageResources(pd, ld, dispM->graphicPipeline->swapchain); // non-MSAA
    createGeometryBufferResources(pd, ld, dispM->graphicPipeline->swapchain);
    createNormalsBufferResources(pd, ld, dispM->graphicPipeline->swapchain);
    createGBufferSampler(ld->device, dispM->graphicPipeline->swapchain);

    // Render passes are owned by the render graph; pipelines get them in
    // metaInitPipelines (post-rgCompile).

    dsm->scImageViewLen = dsm->scImagesLen;
    for (uint32_t i = 0; i < dsm->scImagesLen; i++)
    {
        dsm->p_scImageView[i] = createImageView(
            dsm, ld->device, dsm->currentSurfaceFormat->format,
            dsm->p_scImages[i],
            1, VK_IMAGE_ASPECT_COLOR_BIT);
    }

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
    for (uint32_t compIndex = 0; compIndex < lightVolumeGraphicPipeline->computePipelineCount; compIndex++)
    {
        lightVolumeGraphicPipeline->computeP[compIndex].commandPool = lightVolumeGraphicPipeline->commandPool;
    }

    createCommandBuffer(gdp, &ld->device, gdp->maxFrames);
    createCommandBuffer(lightVolumeGraphicPipeline, &ld->device,
                        lightVolumeGraphicPipeline->maxFrames);
    createCommandBufferWithCompute(particleGraphicPipeline, &ld->device,
                                   particleGraphicPipeline->maxFrames);

    // Publish outputs
    ctx.pd  = pd;
    ctx.ld  = ld;
    ctx.dsm = dsm;
}
