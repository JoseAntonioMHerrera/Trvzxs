#include "passCallbacks.h"
#include "passData.h"
#include "geometryPipelineTypes.h"
#include "deviceManagmentTypes.h"
#include "computePipeline.h"
#include "particle.h"
#include "bindless.h"
#include "bindlessPushConstants.h"
#include <math.h>

// ---------------------------------------------------------------------------
// Depth prepass — vertex-only, populates depth buffer for Early-Z in color pass
// ---------------------------------------------------------------------------

void executeDepthPrepass(VkCommandBuffer cmd, const RGPass* pass, void* userData)
{
    DepthPrepassData*         data     = (DepthPrepassData*) userData;
    GraphicPipelineToSurface* depthGdp = data->depthPipeline;
    GraphicPipelineToSurface* modelGdp = data->modelPipeline;
    uint32_t                  frame    = data->dm->currentFrame;

    // Bind the depth-only pipeline (vertex shader only, no fragment)
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, *depthGdp->pipeline);

    VkViewport viewport = {
        .x        = 0.0f,
        .y        = 0.0f,
        .width    = (float) modelGdp->swapchain->currentExtent2D.width,
        .height   = (float) modelGdp->swapchain->currentExtent2D.height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor = {
        .offset = {0, 0},
        .extent = modelGdp->swapchain->currentExtent2D,
    };
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    // A11 — bind bindless global set (0) + depth prepass per-pass set (1).
    VkDescriptorSet depthSets[2] = {depthGdp->bindlessGlobalSet, depthGdp->perPassSets[frame]};
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, *depthGdp->pipelineLayout,
                            0, 2, depthSets, 0, VK_NULL_HANDLE);

    // Bind the same vertex + index buffers as the model pass
    VkDeviceSize   offsets[]     = {0};
    const VkBuffer localBuffer[] = {*modelGdp->deviceBuffer->buffer};
    vkCmdBindVertexBuffers(cmd, 0, 1, localBuffer, offsets);
    vkCmdBindIndexBuffer(cmd, *modelGdp->deviceBuffer->indexBuffer, 0, VK_INDEX_TYPE_UINT32);

    // LOD draw loop — both depth prepass and model pass MUST issue identical
    // draws in the same order so depth prepass and main pass operate on the
    // same geometry. If you change one loop, change the matching one in
    // executeModelPass.
    const SCENE* scene = modelGdp->scene;
    for (int i = 0; i < scene->modelsManager.modelLen; i++)
    {
        Model3D* model = &scene->modelsManager.pModel[i];
        if (model->alive != 1) continue;
        uint32_t typeBase  = scene->modelsManager.instanceOffset.items[i];
        uint32_t lodOffset = 0;
        for (int k = 0; k < MODEL_LOD_COUNT; k++)
        {
            uint32_t count = model->lodLiveInstanceCount[k];
            if (count == 0) continue;
            BindlessPushConstants pc = {
                .textureIdx   = depthGdp->bindlessTextureHandle,
                .transformIdx = depthGdp->bindlessTransformHandles[frame],
            };
            vkCmdPushConstants(cmd, *depthGdp->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,
                               0, sizeof(pc), &pc);
            vkCmdDrawIndexed(cmd,
                             model->lod[k].indexCount,
                             count,
                             model->lod[k].indexOffset,
                             model->lod[k].vertexOffset,
                             typeBase + lodOffset);
            lodOffset += count;
        }
    }
}

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

    // A11 — bind bindless global set (0) + model per-pass set (1).
    VkDescriptorSet modelSets[2] = {gdp->bindlessGlobalSet, gdp->perPassSets[frame]};
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, *gdp->pipelineLayout, 0, 2,
                            modelSets, 0, VK_NULL_HANDLE);
    vkCmdBindVertexBuffers(cmd, 0, 1, localBuffer, offsets);
    vkCmdBindIndexBuffer(cmd, *gdp->deviceBuffer->indexBuffer, 0, VK_INDEX_TYPE_UINT32);

    // LOD draw loop — must mirror executeDepthPrepass exactly.
    // See comment there for the coupling invariant.
    const SCENE* scene = gdp->scene;
    for (int i = 0; i < scene->modelsManager.modelLen; i++)
    {
        Model3D* model = &scene->modelsManager.pModel[i];
        if (model->alive != 1) continue;
        uint32_t typeBase  = scene->modelsManager.instanceOffset.items[i];
        uint32_t lodOffset = 0;
        for (int k = 0; k < MODEL_LOD_COUNT; k++)
        {
            uint32_t count = model->lodLiveInstanceCount[k];
            if (count == 0) continue;
            BindlessPushConstants pc = {
                .textureIdx   = gdp->bindlessTextureHandle,
                .transformIdx = gdp->bindlessTransformHandles[frame],
            };
            vkCmdPushConstants(cmd, *gdp->pipelineLayout,
                               VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                               0, sizeof(pc), &pc);
            vkCmdDrawIndexed(cmd,
                             model->lod[k].indexCount,
                             count,
                             model->lod[k].indexOffset,
                             model->lod[k].vertexOffset,
                             typeBase + lodOffset);
            lodOffset += count;
        }
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

// ---------------------------------------------------------------------------
// Particle compute pass — extracted from recordParticleCommandBuffer()
// ---------------------------------------------------------------------------

void executeParticleComputePass(VkCommandBuffer cmd, const RGPass* pass, void* userData)
{
    ComputePassData*          data     = (ComputePassData*) userData;
    GraphicPipelineToSurface* pipeline = data->pipeline;
    ComputePipeline*          computeP = pipeline->computeP;
    uint32_t                  frame    = data->dm->currentFrame;

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, *computeP->pipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, *pipeline->pipelineLayout, 0, 1,
                            &pipeline->descriptorSet[frame], 0, 0);

    float deltaTime = *data->deltaTime;
    vkCmdUpdateBuffer(cmd, *computeP->deviceBuffer->pUbo->uniformBuffers, 0, sizeof(float), &deltaTime);

    // Transfer→compute barrier for the UBO update
    VkBufferMemoryBarrier uboBarrier = {
        .sType         = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
        .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT,
        .buffer        = *computeP->deviceBuffer->pUbo->uniformBuffers,
        .offset        = 0,
        .size          = sizeof(float),
    };
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 1,
                         &uboBarrier, 0, NULL);

    uint32_t xWorkDivision = (uint32_t) floor(pipeline->scene->particleManager.packedParticleInstanceLen / 256);
    vkCmdDispatch(cmd, xWorkDivision ? xWorkDivision : 1, 1, 1);
}
