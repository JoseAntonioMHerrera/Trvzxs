#include "clusterGPU.h"
#include "clusterGrid.h"
#include "ssboBuffer.h"
#include <cglm/cglm.h>

void rebuildClusterAABBsOnResize(LogicalDevice *ld,
                                 GraphicPipelineToSurface *lightPipeline,
                                 GraphicSwapchain *dsm)
{
    if (lightPipeline == NULL || lightPipeline->computePipelineCount == 0)
        return;

    ComputePipeline *lightCullCP = &lightPipeline->computeP[0];
    if (lightCullCP->deviceBuffer == NULL || lightCullCP->deviceBuffer->ssbos == NULL)
        return;

    // Mirror updateFrameGlobals's proj construction.
    float aspect = dsm->currentExtent2D.width / (float) dsm->currentExtent2D.height;
    mat4  proj;
    glm_perspective(glm_rad(45.0f), aspect, 0.1f, 30000.0f, proj);
    proj[1][1] *= -1.0f;

    uploadClusterAABBs(ld, lightCullCP->deviceBuffer->ssbos,
                       lightCullCP->commandPool, proj, 0.1f, 30000.0f);
}
