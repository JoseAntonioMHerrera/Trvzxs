#include "camera.h"
#include "computePipeline.h"
#include "deletionQueue.h"
#include "displayManager.h"
#include "timelineSync.h"
#include "particle.h"
#include "renderGraph.h"
#include "transformations.h"
#include "mat4.h"
#include "affine.h"
#include <assert.h>
void cleanupSyncObjects(VkDevice device, GraphicPipelineToSurface* gdp)
{
    for (size_t i = 0; i < gdp->maxFrames; i++)
    {
        vkDestroySemaphore(device, gdp->renderEndFlag[i], NULL);
        vkDestroySemaphore(device, gdp->imgViewSemaphore[i], NULL);
    }
}

void createSDLSurface(DisplayManager* dm, VkInstance* instance, SDL_Window* window)
{
    SDL_bool result_surface_creation = SDL_Vulkan_CreateSurface(window, *instance, &dm->surface);
    assert(dm->surface);
    assert(*instance);

    assert(result_surface_creation == SDL_TRUE);
}


void updateSSBODescriptorSets(VkDevice device, GraphicPipelineToSurface* particleGraphics, uint32_t currentFrame,
                              uint32_t maxFrames)
{
    // Calculate previous frame index with wraparound
    uint32_t prevFrame = (currentFrame - 1 + maxFrames) % maxFrames;

    // Update SSBO descriptor bindings for previous and current frame
    VkDescriptorBufferInfo prevSSBOBufferInfo = {
        .buffer = particleGraphics->computeP->deviceBuffer->ssbos->buffers[prevFrame],
        .offset = 0,
        .range  = sizeof(ParticleInstance) * particleGraphics->scene->particleManager.packedParticleInstanceLen,
    };

    VkDescriptorBufferInfo currentSSBOBufferInfo = {
        .buffer = particleGraphics->computeP->deviceBuffer->ssbos->buffers[currentFrame],
        .offset = 0,
        .range  = sizeof(ParticleInstance) * particleGraphics->scene->particleManager.packedParticleInstanceLen,
    };

    // Create descriptor write updates for both SSBO bindings
    VkWriteDescriptorSet descriptorWrites[] = {
        {
            .sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet           = particleGraphics->descriptorSet[currentFrame],
            .dstBinding       = 2, // Previous frame SSBO binding
            .dstArrayElement  = 0,
            .descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount  = 1,
            .pBufferInfo      = &prevSSBOBufferInfo,
            .pImageInfo       = VK_NULL_HANDLE,
            .pTexelBufferView = VK_NULL_HANDLE,
        },
        {
            .sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet           = particleGraphics->descriptorSet[currentFrame],
            .dstBinding       = 3, // Current frame SSBO binding
            .dstArrayElement  = 0,
            .descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount  = 1,
            .pBufferInfo      = &currentSSBOBufferInfo,
            .pImageInfo       = VK_NULL_HANDLE,
            .pTexelBufferView = VK_NULL_HANDLE,
        },
    };

    // Update the descriptor sets
    vkUpdateDescriptorSets(device, 2, descriptorWrites, 0, VK_NULL_HANDLE);
}

void drawFrame(PhysicalDevice* pd, LogicalDevice* ld, DisplayManager* dm, GraphicPipelineToSurface* gdp,
               GraphicPipelineToSurface* particleGraphics, GraphicPipelineToSurface* lightGraphics, VkDevice device,
               SDL_Window* window, float deltaTime, Camera3D camera, RenderGraph* rg)
{
    FrameSync* sync = &ld->frameSync;

    // CPU pacing: wait for frame N-maxFrames to finish on the graphics timeline
    if (sync->graphicsValue >= sync->maxFramesInFlight)
    {
        uint64_t            waitValue = sync->graphicsValue - sync->maxFramesInFlight + 1;
        VkSemaphoreWaitInfo waitInfo  = {
            .sType          = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
            .semaphoreCount = 1,
            .pSemaphores    = &sync->graphicsTimeline,
            .pValues        = &waitValue,
        };
        vkWaitSemaphores(device, &waitInfo, UINT64_MAX);
    }

    // GPU finished with this frame slot's commands.
    // Safe to destroy any resources enqueued MAX_FRAMES_IN_FLIGHT frames ago.
    flushDeletionQueue(&ld->deletionQueue, ld->device, ld->allocator);
    advanceDeletionQueue(&ld->deletionQueue);

    updateSSBODescriptorSets(ld->device, particleGraphics, dm->currentFrame, particleGraphics->maxFrames);

    uint32_t aquireImageIndex = dm->currentFrame;

    VkResult nextImageResult =
        vkAcquireNextImageKHR(device, dm->graphicPipeline->swapchain->VkSwapchain, UINT64_MAX,
                              gdp->imgViewSemaphore[dm->currentFrame], VK_NULL_HANDLE, &aquireImageIndex);

    switch (nextImageResult)
    {
    case VK_SUCCESS:
        break;
    case VK_SUBOPTIMAL_KHR:
    case VK_ERROR_OUT_OF_DATE_KHR:
        recreateSwapChain(pd, ld, window, rg);
        dm->currentFrame = (dm->currentFrame + 1) % gdp->maxFrames;
        return;
    default:
        exit(1);
    }

    // --- Command recording ---
    VkCommandBuffer graphicsCB = gdp->commandBuffer[dm->currentFrame];
    VkCommandBuffer computeCB  = particleGraphics->computeP->commandBuffer[dm->currentFrame];

    VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };

    vkBeginCommandBuffer(graphicsCB, &beginInfo);
    vkBeginCommandBuffer(computeCB, &beginInfo);

    // Update per-frame resource handles
    rg->resources[0].image     = gdp->swapchain->p_scImages[dm->currentFrame];
    rg->resources[0].imageView = gdp->swapchain->p_scImageView[dm->currentFrame];

    uint32_t prevFrame = (dm->currentFrame - 1 + particleGraphics->maxFrames) % particleGraphics->maxFrames;
    rg->resources[4].buffer = particleGraphics->computeP->deviceBuffer->ssbos->buffers[prevFrame];
    rg->resources[5].buffer = particleGraphics->computeP->deviceBuffer->ssbos->buffers[dm->currentFrame];

    rg->deltaTime = deltaTime;

    rgExecute(rg, graphicsCB, computeCB, dm->currentFrame);

    // Transition swapchain image to present layout (on graphics CB)
    transitionImageLayout(NULL, VK_NULL_HANDLE,
                          gdp->swapchain->p_scImages[dm->currentFrame],
                          gdp->swapchain->currentSurfaceFormat->format,
                          VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                          VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, 1,
                          graphicsCB);

    vkEndCommandBuffer(graphicsCB);
    vkEndCommandBuffer(computeCB);

    // Update UBOs (CPU-side memcpy — no GPU sync needed)
    MODEL_VIEW_PROJECTION* uboModels =
        updateModelUniformBuffers(dm->graphicPipeline->swapchain, deltaTime, camera, dm->graphicPipeline[0].scene);
    memcpy(gdp->deviceBuffer->pUbo->uniformBuffersMapped[dm->currentFrame], uboModels,
           sizeof(MODEL_VIEW_PROJECTION) * dm->graphicPipeline[0].scene->modelsManager.modelInstanceLen);

    if (dm->graphicPipeline[0].scene->modelsManager.modelInstanceLen == 0)
    {
        MODEL_VIEW_PROJECTION cameraOnly;
        glm_mat4_identity(cameraOnly.model);
        glm_lookat(camera.eye, camera.center, camera.up, cameraOnly.view);
        float aspectRatio = dm->graphicPipeline->swapchain->currentExtent2D.width /
                            (float) dm->graphicPipeline->swapchain->currentExtent2D.height;
        glm_perspective(glm_rad(45.0f), aspectRatio, 2.0f, 500.0f, cameraOnly.proj);
        cameraOnly.proj[1][1] *= -1;
        memcpy(gdp->deviceBuffer->pUbo->uniformBuffersMapped[dm->currentFrame], &cameraOnly, sizeof(cameraOnly));
    }

    LightTransformations* uboLights =
        updateLightUniformBuffers(dm->graphicPipeline->swapchain, deltaTime, camera, dm->graphicPipeline[2].scene);
    memcpy(lightGraphics->deviceBuffer->pUbo->uniformBuffersMapped[dm->currentFrame], uboLights,
           sizeof(*uboLights) * dm->graphicPipeline[2].scene->lightManager.lightInstanceCount);

    // Submit both queues with timeline semaphore sync
    rgSubmit(rg, graphicsCB, computeCB, sync,
             gdp->imgViewSemaphore[dm->currentFrame],
             gdp->renderEndFlag[dm->currentFrame]);

    // Present
    VkSemaphore    presentWait[] = {gdp->renderEndFlag[dm->currentFrame]};
    VkSwapchainKHR swapChains[]  = {dm->graphicPipeline->swapchain->VkSwapchain};

    VkPresentInfoKHR presentInfo = {
        .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores    = presentWait,
        .swapchainCount     = 1,
        .pSwapchains        = swapChains,
        .pImageIndices      = &aquireImageIndex,
        .pResults           = NULL,
    };

    VkResult presentResult = vkQueuePresentKHR(*ld->queueManager.presentQueue, &presentInfo);

    dm->currentFrame = (dm->currentFrame + 1) % gdp->maxFrames;
}
