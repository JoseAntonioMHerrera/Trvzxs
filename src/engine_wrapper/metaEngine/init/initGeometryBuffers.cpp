#include "initGeometryBuffers.hpp"

#include <cassert>

extern "C"
{
#include "buffer.h"
#include "commandPool.h"
#include "main_vulkan.h"
}

void metaInitGeometryBuffers(MetaInitContext &ctx)
{
    DeviceManager            *dm                         = ctx.dm;
    DisplayManager           *dispM                      = ctx.dispM;
    PhysicalDevice           *pd                         = ctx.pd;
    LogicalDevice            *ld                         = ctx.ld;
    GraphicPipelineToSurface *gdp                        = ctx.gdp;
    GraphicPipelineToSurface *particleGraphicPipeline    = ctx.particlePipeline;
    GraphicPipelineToSurface *lightVolumeGraphicPipeline = ctx.lightPipeline;

    // Particle vertex + index (packed)
    createParticleBuffer(pd, ld, particleGraphicPipeline->deviceBuffer,
                         particleGraphicPipeline->scene,
                         particleGraphicPipeline->commandPool);
    createParticleIndiceBuffers(
        pd, ld, particleGraphicPipeline->deviceBuffer,
        particleGraphicPipeline->scene,
        particleGraphicPipeline->commandPool);

    // Model vertex + index (packed across all Model3D types)
    createGeometryBuffer(pd, ld, gdp->deviceBuffer, gdp->scene,
                         gdp->commandPool);
    assert(gdp->scene != nullptr);
    createGeometryIndiceBuffer(pd, ld, gdp->deviceBuffer, gdp->scene,
                               gdp->commandPool);

    // Light volume vertex + index (packed across all LightVolume types).
    // Index buffer creation is skipped when there are no indexed lights.
    createLightVolumeVertexBuffer(
        pd, ld, lightVolumeGraphicPipeline->deviceBuffer,
        lightVolumeGraphicPipeline->scene,
        lightVolumeGraphicPipeline->commandPool);
    if (lightVolumeGraphicPipeline->scene->lightManager.packedLightIndex.itemsLen > 0)
    {
        createLightVolumeIndiceBuffers(
            pd, ld, lightVolumeGraphicPipeline->deviceBuffer,
            lightVolumeGraphicPipeline->scene,
            lightVolumeGraphicPipeline->commandPool);
    }

    // Sync — per-pipeline semaphores/fences + per-frame sync
    createSyncObjects(gdp, &ld->device);
    createSyncObjects(particleGraphicPipeline, &ld->device);
    createFrameSync(&ld->frameSync, ld->device, gdp->maxFrames);

    // Vulkan validation / debug messenger
    dm->logCallback = createDebugCallbackForVulkan(dm->instance);
    dispM->currentFrame = 0;
}
