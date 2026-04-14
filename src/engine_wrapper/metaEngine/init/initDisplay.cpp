#include "initDisplay.hpp"
#include "scene.hpp" // full definition for scene->getScene()

#include <cstdlib>

extern "C"
{
#include "buffer.h"
#include "commandPool.h"
#include "computePipeline.h"
#include "geometryPipelineTypes.h"
#include "graphicConfiguration.h"
#include "main_vulkan.h"
#include "scene.h"
#include "swapchainManager.h"
}

void metaInitDisplay(MetaInitContext &ctx, Scene *scene, SDL_Window *window)
{
    DeviceManager *dm = ctx.dm;

    // DisplayManager + 4 graphic pipeline slots
    DisplayManager *dispM = (DisplayManager *)malloc(sizeof(DisplayManager));
    dispM->graphicsPipelineLen = 4; // 0 models, 1 particles, 2 lights, 3 depth prepass
    dispM->graphicPipeline = (GraphicPipelineToSurface *)malloc(
        sizeof(*dispM->graphicPipeline) * dispM->graphicsPipelineLen);

    GraphicPipelineToSurface *gdp                         = &dispM->graphicPipeline[0];
    GraphicPipelineToSurface *particleGraphicPipeline     = &dispM->graphicPipeline[1];
    GraphicPipelineToSurface *lightVolumeGraphicPipeline  = &dispM->graphicPipeline[2];
    GraphicPipelineToSurface *depthPrepassGraphicPipeline = &dispM->graphicPipeline[3];

    // Scene fan-out (all 4 pipelines share the same scene struct)
    SCENE *sceneCStruct = scene->getScene();
    gdp->scene                          = sceneCStruct;
    particleGraphicPipeline->scene      = sceneCStruct;
    lightVolumeGraphicPipeline->scene   = sceneCStruct;
    depthPrepassGraphicPipeline->scene  = sceneCStruct;

    // Shared swapchain struct — allocated here, populated by Phase 5.
    gdp->swapchain = (GraphicSwapchain *)malloc(sizeof(*gdp->swapchain));

    // Per-pipeline compute pipeline slots
    particleGraphicPipeline->computePipelineCount = 1;
    particleGraphicPipeline->computeP = (ComputePipeline *)malloc(
        sizeof(*particleGraphicPipeline->computeP));

    depthPrepassGraphicPipeline->computePipelineCount = 2;
    depthPrepassGraphicPipeline->computeP = (ComputePipeline *)malloc(
        sizeof(*depthPrepassGraphicPipeline->computeP)
        * depthPrepassGraphicPipeline->computePipelineCount);

    // Per-pipeline DeviceBufferManager slots
    gdp->deviceBuffer = (DeviceBufferManager *)calloc(1, sizeof(*gdp->deviceBuffer));
    particleGraphicPipeline->deviceBuffer = (DeviceBufferManager *)calloc(
        1, sizeof(*particleGraphicPipeline->deviceBuffer));
    particleGraphicPipeline->computeP->deviceBuffer = (DeviceBufferManager *)calloc(
        1, sizeof(*particleGraphicPipeline->computeP->deviceBuffer));
    lightVolumeGraphicPipeline->deviceBuffer = (DeviceBufferManager *)calloc(
        1, sizeof(*lightVolumeGraphicPipeline->deviceBuffer));
    depthPrepassGraphicPipeline->deviceBuffer = (DeviceBufferManager *)calloc(
        1, sizeof(*depthPrepassGraphicPipeline->deviceBuffer));

    for (uint32_t compIndex = 0; compIndex < particleGraphicPipeline->computePipelineCount; compIndex++)
    {
        particleGraphicPipeline->computeP[compIndex].deviceBuffer = (DeviceBufferManager *)calloc(
            1, sizeof(*particleGraphicPipeline->computeP[compIndex].deviceBuffer));
    }
    for (uint32_t compIndex = 0; compIndex < depthPrepassGraphicPipeline->computePipelineCount; compIndex++)
    {
        depthPrepassGraphicPipeline->computeP[compIndex].deviceBuffer = (DeviceBufferManager *)calloc(
            1, sizeof(*depthPrepassGraphicPipeline->computeP[compIndex].deviceBuffer));
    }

    // Per-pipeline CommandPools slots (actual pools are created in Phase 5)
    gdp->commandPool                         = (CommandPools *)malloc(sizeof(*gdp->commandPool));
    particleGraphicPipeline->commandPool     = (CommandPools *)malloc(sizeof(*particleGraphicPipeline->commandPool));
    lightVolumeGraphicPipeline->commandPool  = (CommandPools *)malloc(sizeof(*lightVolumeGraphicPipeline->commandPool));
    depthPrepassGraphicPipeline->commandPool = (CommandPools *)malloc(sizeof(*depthPrepassGraphicPipeline->commandPool));
    particleGraphicPipeline->computeP->commandPool = particleGraphicPipeline->commandPool;

    for (uint32_t compIndex = 0; compIndex < particleGraphicPipeline->computePipelineCount; compIndex++)
    {
        particleGraphicPipeline->computeP[compIndex].commandPool = (CommandPools *)malloc(sizeof(CommandPools));
    }
    for (uint32_t compIndex = 0; compIndex < depthPrepassGraphicPipeline->computePipelineCount; compIndex++)
    {
        depthPrepassGraphicPipeline->computeP[compIndex].commandPool = (CommandPools *)malloc(sizeof(CommandPools));
    }

    // Seed per-pipeline maxFrames + dsm->scImagesLen with the desired in-flight
    // count. Phase 5 reads back the actual value negotiated by createSwapChain.
    gdp->maxFrames                         = MAX_FRAMES_IN_FLIGHT;
    particleGraphicPipeline->maxFrames     = MAX_FRAMES_IN_FLIGHT;
    lightVolumeGraphicPipeline->maxFrames  = MAX_FRAMES_IN_FLIGHT;
    depthPrepassGraphicPipeline->maxFrames = MAX_FRAMES_IN_FLIGHT;
    gdp->swapchain->scImagesLen            = MAX_FRAMES_IN_FLIGHT;

    // SDL surface + Vulkan device selection
    createSDLSurface(dispM, dm->instance, window);
    initDeviceManager(dm->instance, dm, dispM);

    // Publish outputs
    ctx.dispM                = dispM;
    ctx.gdp                  = gdp;
    ctx.particlePipeline     = particleGraphicPipeline;
    ctx.lightPipeline        = lightVolumeGraphicPipeline;
    ctx.depthPrepassPipeline = depthPrepassGraphicPipeline;
}
