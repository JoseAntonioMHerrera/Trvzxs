#include "initRenderGraph.hpp"

extern "C"
{
#include "image.h"
#include "passCallbacks.h"
#include "ssboBuffer.h"
}

void metaInitRenderGraph(MetaInitContext   &ctx,
                         RenderGraph       *renderGraph,
                         DepthPrepassData  *depthPrepassData,
                         ModelPassData     *modelPassData,
                         LightPassData     *lightPassData,
                         ParticlePassData  *particlePassData,
                         ComputePassData   *computePassData)
{
    DisplayManager           *dispM                       = ctx.dispM;
    PhysicalDevice           *pd                          = ctx.pd;
    LogicalDevice            *ld                          = ctx.ld;
    GraphicSwapchain         *dsm                         = ctx.dsm;
    GraphicPipelineToSurface *gdp                         = ctx.gdp;
    GraphicPipelineToSurface *particleGraphicPipeline     = ctx.particlePipeline;
    GraphicPipelineToSurface *lightVolumeGraphicPipeline  = ctx.lightPipeline;
    GraphicPipelineToSurface *depthPrepassGraphicPipeline = ctx.depthPrepassPipeline;

    // === RENDER GRAPH SETUP ===
    rgInit(renderGraph, ld->device, pd,
           dsm->currentExtent2D.width, dsm->currentExtent2D.height,
           dsm->scImagesLen);
    renderGraph->debugLog = true;
    rgSetQueues(renderGraph,
                *ld->queueManager.graphicsQueue,
                *ld->queueManager.computeQueue,
                pd->queueFamilyIndices.graphicsFamily,
                pd->queueFamilyIndices.computeFamily);

    // Register external resources
    uint32_t rColor = rgAddExternalResource(renderGraph, (RGResource){
        .type       = RG_IMAGE,
        .format     = dsm->currentSurfaceFormat->format,
        .samples    = VK_SAMPLE_COUNT_1_BIT,
        .sizeMode   = RG_SIZE_SWAPCHAIN,
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .image      = dsm->p_scImages[0],
        .imageView  = dsm->p_scImageView[0],
        .perFrameImageViews = dsm->p_scImageView,
        .isExternal = true,
    });

    uint32_t rDepth = rgAddExternalResource(renderGraph, (RGResource){
        .type       = RG_IMAGE,
        .format     = findDepthImageFormat(pd->gpu),
        .samples    = VK_SAMPLE_COUNT_1_BIT,
        .sizeMode   = RG_SIZE_SWAPCHAIN,
        .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
        .image      = dsm->depthImage.common->image,
        .imageView  = dsm->depthImage.common->imageView,
        .isExternal = true,
    });

    uint32_t rGBuffer = rgAddExternalResource(renderGraph, (RGResource){
        .type       = RG_IMAGE,
        .format     = findGeometryBufferFormat(pd->gpu),
        .samples    = VK_SAMPLE_COUNT_1_BIT,
        .sizeMode   = RG_SIZE_SWAPCHAIN,
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .image      = dsm->gBuffer->image,
        .imageView  = dsm->gBuffer->imageView,
        .isExternal = true,
    });

    uint32_t rNormals = rgAddExternalResource(renderGraph, (RGResource){
        .type       = RG_IMAGE,
        .format     = findNormalsBufferFormat(pd->gpu),
        .samples    = VK_SAMPLE_COUNT_1_BIT,
        .sizeMode   = RG_SIZE_SWAPCHAIN,
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .image      = dsm->nBuffer->image,
        .imageView  = dsm->nBuffer->imageView,
        .isExternal = true,
    });

    // Pass 0: Depth prepass — vertex-only, populates depth buffer
    uint32_t pDepthPrepass = rgAddPass(renderGraph, RG_PASS_GRAPHICS);
    rgPassSetName(renderGraph, pDepthPrepass, RG_PASS_NAME_DEPTH_PREPASS);
    rgPassAddOutput(renderGraph, pDepthPrepass, (RGResourceUsage){
        .resourceIndex = rDepth,
        .layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        .accessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        .stageMask  = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
                      VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
        .access     = RG_WRITE,
    });

    *depthPrepassData = (DepthPrepassData){
        .depthPipeline = depthPrepassGraphicPipeline,
        .modelPipeline = gdp,
        .dm            = dispM,
    };
    rgPassSetCallback(renderGraph, pDepthPrepass, executeDepthPrepass, depthPrepassData);

    // Pass 1: Models — reads depth (prepass populated it), writes G-buffer + normals
    uint32_t pModels = rgAddPass(renderGraph, RG_PASS_GRAPHICS);
    rgPassSetName(renderGraph, pModels, RG_PASS_NAME_MODELS);
    rgPassAddOutput(renderGraph, pModels, (RGResourceUsage){
        .resourceIndex = rGBuffer,
        .layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .accessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .stageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .access     = RG_WRITE,
    });
    rgPassAddOutput(renderGraph, pModels, (RGResourceUsage){
        .resourceIndex = rNormals,
        .layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .accessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .stageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .access     = RG_WRITE,
    });
    // Depth: input triggers a barrier from the prepass (depth write → read),
    // output makes the render graph create a pDepthStencilAttachment so the
    // hardware depth test works. The pipeline has depthWriteEnable=FALSE so
    // no actual depth writes occur despite the output declaration.
    rgPassAddInput(renderGraph, pModels, (RGResourceUsage){
        .resourceIndex = rDepth,
        .layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
        .accessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
        .stageMask  = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
        .access     = RG_READ,
    });
    // Depth is declared as an output so the render graph builds a depth-stencil
    // attachment (required for the hardware depth test to function). The pipeline
    // has depthWriteEnable=FALSE so no shader-issued writes occur — BUT the
    // renderpass-end implicit layout transition (READ_ONLY → ATTACHMENT) is
    // treated by Vulkan sync validation as a DEPTH_STENCIL_ATTACHMENT_WRITE in
    // LATE_FRAGMENT_TESTS. Declaring WRITE here makes pLights' pre-barrier
    // cover that implicit write, fixing SYNC-HAZARD-WRITE-AFTER-WRITE on rDepth.
    rgPassAddOutput(renderGraph, pModels, (RGResourceUsage){
        .resourceIndex = rDepth,
        .layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        .accessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                      VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        .stageMask  = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
                      VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
        .access     = RG_READ,
    });

    *modelPassData = (ModelPassData){
        .pipeline = gdp,
        .dm       = dispM,
    };
    rgPassSetCallback(renderGraph, pModels, executeModelPass, modelPassData);

    // Pass 1: Lights — reads G-buffer + normals, writes color
    uint32_t pLights = rgAddPass(renderGraph, RG_PASS_GRAPHICS);
    rgPassSetName(renderGraph, pLights, RG_PASS_NAME_LIGHTS);
    rgPassAddInput(renderGraph, pLights, (RGResourceUsage){
        .resourceIndex = rGBuffer,
        .layout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        .accessMask = VK_ACCESS_SHADER_READ_BIT,
        .stageMask  = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        .access     = RG_READ,
    });
    rgPassAddInput(renderGraph, pLights, (RGResourceUsage){
        .resourceIndex = rNormals,
        .layout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        .accessMask = VK_ACCESS_SHADER_READ_BIT,
        .stageMask  = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        .access     = RG_READ,
    });
    rgPassAddInput(renderGraph, pLights, (RGResourceUsage){
        .resourceIndex = rDepth,
        .layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
        .accessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
        .stageMask  = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
        .access     = RG_READ,
    });
    rgPassAddOutput(renderGraph, pLights, (RGResourceUsage){
        .resourceIndex = rColor,
        .layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .accessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .stageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .access     = RG_WRITE,
    });

    *lightPassData = (LightPassData){
        .pipeline = lightVolumeGraphicPipeline,
        .dm       = dispM,
    };
    rgPassSetCallback(renderGraph, pLights, executeLightPass, lightPassData);

    // Pass 2: Particles — reads + writes color
    uint32_t pParticles = rgAddPass(renderGraph, RG_PASS_GRAPHICS);
    rgPassSetName(renderGraph, pParticles, RG_PASS_NAME_PARTICLES);
    rgPassAddInput(renderGraph, pParticles, (RGResourceUsage){
        .resourceIndex = rColor,
        .layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .accessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT,
        .stageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .access     = RG_READ,
    });
    rgPassAddOutput(renderGraph, pParticles, (RGResourceUsage){
        .resourceIndex = rColor,
        .layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .accessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .stageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .access     = RG_WRITE,
    });

    *particlePassData = (ParticlePassData){
        .pipeline = particleGraphicPipeline,
        .dm       = dispM,
    };
    rgPassSetCallback(renderGraph, pParticles, executeParticlePass, particlePassData);

    // Resources 4-5: Particle SSBO double buffers (external, per-frame)
    SSBOBufferManager* ssbos = particleGraphicPipeline->computeP->deviceBuffer->ssbos;
    uint32_t rSSBOPrev = rgAddExternalResource(renderGraph, (RGResource){
        .type       = RG_BUFFER,
        .buffer     = ssbos->buffers[0],
        .bufferSize = VK_WHOLE_SIZE,
        .isExternal = true,
    });
    uint32_t rSSBOCurr = rgAddExternalResource(renderGraph, (RGResource){
        .type       = RG_BUFFER,
        .buffer     = ssbos->buffers[1],
        .bufferSize = VK_WHOLE_SIZE,
        .isExternal = true,
    });

    // Pass 3: Particle compute — reads prev SSBO, writes curr SSBO
    uint32_t pParticleCompute = rgAddPass(renderGraph, RG_PASS_COMPUTE);
    rgPassSetName(renderGraph, pParticleCompute, RG_PASS_NAME_PARTICLE_COMPUTE);
    rgPassSetQueue(renderGraph, pParticleCompute, RG_QUEUE_COMPUTE);
    rgPassAddInput(renderGraph, pParticleCompute, (RGResourceUsage){
        .resourceIndex = rSSBOPrev,
        .accessMask    = VK_ACCESS_SHADER_READ_BIT,
        .stageMask     = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        .access        = RG_READ,
    });
    rgPassAddOutput(renderGraph, pParticleCompute, (RGResourceUsage){
        .resourceIndex = rSSBOCurr,
        .accessMask    = VK_ACCESS_SHADER_WRITE_BIT,
        .stageMask     = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        .access        = RG_WRITE,
    });

    *computePassData = (ComputePassData){
        .pipeline  = particleGraphicPipeline,
        .dm        = dispM,
        .pd        = pd,
        .ld        = ld,
        .deltaTime = &renderGraph->deltaTime,
    };
    rgPassSetCallback(renderGraph, pParticleCompute, executeParticleComputePass, computePassData);

    // Particle render pass also reads the current SSBO (vertex input)
    rgPassAddInput(renderGraph, pParticles, (RGResourceUsage){
        .resourceIndex = rSSBOCurr,
        .accessMask    = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,
        .stageMask     = VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
        .access        = RG_READ,
    });

    // Compile: topological sort + barrier generation + render pass creation
    rgCompile(renderGraph);

    // Publish pass indices for Phase 2 (initPipelines)
    ctx.pDepthPrepass    = pDepthPrepass;
    ctx.pModels          = pModels;
    ctx.pLights          = pLights;
    ctx.pParticles       = pParticles;
    ctx.pParticleCompute = pParticleCompute;
}
