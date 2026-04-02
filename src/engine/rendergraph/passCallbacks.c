#include "passCallbacks.h"
#include "passData.h"
#include "geometryPipelineTypes.h"
#include "deviceManagmentTypes.h"

// ---------------------------------------------------------------------------
// Model pass — extracted from startRenderPass() subpass 0
// ---------------------------------------------------------------------------

void executeModelPass(VkCommandBuffer cmd, const RGPass* pass, void* userData)
{
    ModelPassData*            data  = (ModelPassData*) userData;
    GraphicPipelineToSurface* gdp   = data->pipeline;
    uint32_t                  frame = data->dm->currentFrame;

    VkDeviceSize   offsets[]     = {0};
    const VkBuffer localBuffer[] = {*gdp->deviceBuffer->buffer};

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, *gdp->pipeline);

    VkViewport viewport = {
        .x        = 0.0f,
        .y        = 0.0f,
        .width    = (float) gdp->swapchain->currentExtent2D.width,
        .height   = (float) gdp->swapchain->currentExtent2D.height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor = {
        .offset = {0, 0},
        .extent = gdp->swapchain->currentExtent2D,
    };
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, *gdp->pipelineLayout, 0, 1,
                            &gdp->descriptorSet[frame], 0, VK_NULL_HANDLE);
    vkCmdBindVertexBuffers(cmd, 0, 1, localBuffer, offsets);
    vkCmdBindIndexBuffer(cmd, *gdp->deviceBuffer->indexBuffer, 0, VK_INDEX_TYPE_UINT32);

    const SCENE* scene = gdp->scene;
    for (int i = 0; i < scene->modelsManager.modelLen; i++)
    {
        if (scene->modelsManager.pModel[i].liveInstanceCount > 0)
            vkCmdDrawIndexed(cmd, scene->modelsManager.pModel[i].indices.itemsLen,
                             scene->modelsManager.pModel[i].liveInstanceCount,
                             scene->modelsManager.pModel[i].indiceOffset,
                             scene->modelsManager.pModel[i].vertexOffset,
                             scene->modelsManager.instanceOffset.items[i]);
    }
}

// ---------------------------------------------------------------------------
// Light pass — extracted from startRenderPass() subpass 1
// ---------------------------------------------------------------------------

void executeLightPass(VkCommandBuffer cmd, const RGPass* pass, void* userData)
{
    LightPassData*            data  = (LightPassData*) userData;
    GraphicPipelineToSurface* lp    = data->pipeline;
    uint32_t                  frame = data->dm->currentFrame;

    VkDeviceSize offsets[] = {0};

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, *lp->pipeline);

    VkViewport viewport = {
        .x        = 0.0f,
        .y        = 0.0f,
        .width    = (float) lp->swapchain->currentExtent2D.width,
        .height   = (float) lp->swapchain->currentExtent2D.height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor = {
        .offset = {0, 0},
        .extent = lp->swapchain->currentExtent2D,
    };
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, *lp->pipelineLayout, 0, 1,
                            &lp->descriptorSet[frame], 0, VK_NULL_HANDLE);
    vkCmdBindVertexBuffers(cmd, 0, 1, lp->deviceBuffer->buffer, offsets);

    const SCENE* scene = lp->scene;
    for (int i = 0; i < scene->lightManager.lightLen; i++)
    {
        if (scene->lightManager.pLight[i].liveInstanceCount == 0)
            continue;

        vkCmdDraw(cmd, scene->lightManager.pLight[i].model.meshManager.totalMeshVerticesLen,
                  scene->lightManager.pLight[i].liveInstanceCount,
                  scene->lightManager.pLight[i].model.vertexOffset,
                  scene->lightManager.instanceOffset.items[i]);
    }
}

// ---------------------------------------------------------------------------
// Particle pass — extracted from startRenderPass() subpass 2
// ---------------------------------------------------------------------------

void executeParticlePass(VkCommandBuffer cmd, const RGPass* pass, void* userData)
{
    ParticlePassData*         data  = (ParticlePassData*) userData;
    GraphicPipelineToSurface* pp    = data->pipeline;
    uint32_t                  frame = data->dm->currentFrame;

    VkDeviceSize offsets[] = {0};

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, *pp->pipeline);

    VkViewport viewport = {
        .x        = 0.0f,
        .y        = 0.0f,
        .width    = (float) pp->swapchain->currentExtent2D.width,
        .height   = (float) pp->swapchain->currentExtent2D.height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor = {
        .offset = {0, 0},
        .extent = pp->swapchain->currentExtent2D,
    };
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, *pp->pipelineLayout, 0, 1,
                            &pp->descriptorSet[frame], 0, VK_NULL_HANDLE);
    vkCmdBindVertexBuffers(cmd, 0, 1, pp->deviceBuffer->buffer, offsets);

    if (!pp->deviceBuffer->indexBuffer || *pp->deviceBuffer->indexBuffer == VK_NULL_HANDLE)
        return;
    vkCmdBindIndexBuffer(cmd, *pp->deviceBuffer->indexBuffer, 0, VK_INDEX_TYPE_UINT32);

    const SCENE* scene = pp->scene;
    for (uint32_t g = 0; g < scene->particleManager.particleGroupLen; g++)
    {
        vkCmdDrawIndexed(cmd, scene->particleManager.packedParticleIndicesLen,
                         scene->particleManager.pParticleGroup[g].particleInstancesLen, 0, 0, 0);
    }
}
