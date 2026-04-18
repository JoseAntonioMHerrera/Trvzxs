#include "initStorageBuffers.hpp"

#include <cstdlib>

extern "C"
{
#include "ssboBuffer.h"
#include "clusterGrid.h"
#include <cglm/cglm.h>
}

// Build the same projection matrix that updateFrameGlobals produces, from the
// current swapchain extent. Keep the 45°/0.1/30000 constants in sync with
// uniformBuffer.c::updateFrameGlobals — any change there must mirror here.
static void buildInitialProj(GraphicSwapchain *dsm, mat4 out)
{
    float aspect = dsm->currentExtent2D.width / (float) dsm->currentExtent2D.height;
    glm_perspective(glm_rad(45.0f), aspect, 0.1f, 30000.0f, out);
    out[1][1] *= -1.0f;
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

    // Clustered forward+ Phase 1: cluster AABB SSBOs on the lightCull compute stub.
    // One buffer per frame-in-flight; rebuilt on swapchain recreate (see
    // rebuildClusterAABBsOnResize in clusterGPU.c).
    ComputePipeline *lightCullCP = &lightVolumeGraphicPipeline->computeP[0];
    lightCullCP->deviceBuffer->ssbos =
        (SSBOBufferManager *)calloc(1, sizeof(*lightCullCP->deviceBuffer->ssbos));

    mat4 proj;
    buildInitialProj(ctx.dsm, proj);
    createClusterAABBSSBOs(pd, ld, lightCullCP->maxFrames, lightCullCP->deviceBuffer->ssbos,
                           lightCullCP->commandPool, proj, 0.1f, 30000.0f);
}
