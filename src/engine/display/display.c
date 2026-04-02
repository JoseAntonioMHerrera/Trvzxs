#include "camera.h"
#include "computePipeline.h"
#include "deletionQueue.h"
#include "displayManager.h"
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
        vkDestroyFence(device, gdp->inFlightFence[i], NULL);
    }
}

void createSDLSurface(DisplayManager* dm, VkInstance* instance, SDL_Window* window)
{
    SDL_bool result_surface_creation = SDL_Vulkan_CreateSurface(window, *instance, &dm->surface);
    assert(dm->surface);
    assert(*instance);

    assert(result_surface_creation == SDL_TRUE);
}

void firstFrameInitialPassCompute(PhysicalDevice* pd, LogicalDevice* ld, DisplayManager* dm,
                                  GraphicPipelineToSurface* gdp, GraphicPipelineToSurface* particleGraphics,
                                  VkDevice device)
{
    uint32_t bufferIndex = (dm->currentFrame - 1 + particleGraphics->maxFrames) % particleGraphics->maxFrames;
    // Begin command buffer
    VkCommandBufferBeginInfo beginInfo = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                                          .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};

    // Begin recording
    if (vkBeginCommandBuffer(particleGraphics->computeP->commandBuffer[bufferIndex], &beginInfo) != VK_SUCCESS)
    {
        return;
    }

    // End recording
    if (vkEndCommandBuffer(particleGraphics->computeP->commandBuffer[bufferIndex]) != VK_SUCCESS)
    {
        return;
    }

    // Submit info
    VkSubmitInfo initialSubmitInfo = {.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                                      .waitSemaphoreCount   = 0,
                                      .pWaitSemaphores      = NULL,
                                      .pWaitDstStageMask    = NULL,
                                      .commandBufferCount   = 1,
                                      .pCommandBuffers      = &particleGraphics->computeP->commandBuffer[bufferIndex],
                                      .signalSemaphoreCount = 1,
                                      .pSignalSemaphores =
                                          &particleGraphics->computeP->computeReadySemaphore[bufferIndex]};

    if (vkQueueSubmit(*ld->queueManager.computeQueue, 1, &initialSubmitInfo, VK_NULL_HANDLE) != VK_SUCCESS)
    {
        return;
    }

    // Wait for this initial submission to complete
    vkQueueWaitIdle(*ld->queueManager.computeQueue);
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

// Helper function to print Camera3D
void printCamera3D(Camera3D camera) {}

void drawFrame(PhysicalDevice* pd, LogicalDevice* ld, DisplayManager* dm, GraphicPipelineToSurface* gdp,
               GraphicPipelineToSurface* particleGraphics, GraphicPipelineToSurface* lightGraphics, VkDevice device,
               SDL_Window* window, float deltaTime, Camera3D camera, RenderGraph* rg)
{

    VkSemaphore          signalComputeSemaphores[] = {particleGraphics->computeP->computeEndSemaphore[dm->currentFrame],
                                                      particleGraphics->computeP->computeReadySemaphore[dm->currentFrame]};
    VkPipelineStageFlags waitDstFlagsSemph[]       = {VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT};
    VkSemaphore          waitSemaphoresCompute[]   = {
        particleGraphics->computeP->computeReadySemaphore[(dm->currentFrame - 1 + particleGraphics->maxFrames) %
                                                          particleGraphics->maxFrames]};

    vkResetFences(device, 1, &particleGraphics->computeP->computeInFlightFence[dm->currentFrame]);

    recordParticleCommandBuffer(particleGraphics, pd, ld, *particleGraphics->pipelineLayout,
                                &particleGraphics->descriptorSet[dm->currentFrame], dm->currentFrame, deltaTime);
    VkCommandBuffer submitCommandBuffer[] = {(particleGraphics->computeP->commandBuffer[dm->currentFrame])};
    // COMPUTE SUBMIT WORKLOAD
    VkSubmitInfo computeQueueSubmitInfo = {
        .sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount   = 1,
        .pWaitSemaphores      = waitSemaphoresCompute,
        .pWaitDstStageMask    = waitDstFlagsSemph,
        .commandBufferCount   = 1,
        .pCommandBuffers      = submitCommandBuffer,
        .signalSemaphoreCount = 2,
        .pSignalSemaphores    = signalComputeSemaphores,
        .pNext                = NULL,
    };

    if (vkQueueSubmit(*ld->queueManager.computeQueue, 1, &computeQueueSubmitInfo,
                      particleGraphics->computeP->computeInFlightFence[dm->currentFrame]) != VK_SUCCESS)
    {
    }
    else
    {
    }

    vkWaitForFences(device, 1, &particleGraphics->computeP->computeInFlightFence[dm->currentFrame], VK_TRUE,
                    UINT64_MAX);
    vkWaitForFences(device, 1, &gdp->inFlightFence[dm->currentFrame], VK_TRUE, UINT64_MAX);

    // Fences confirmed GPU finished with this frame's commands.
    // Safe to destroy any resources enqueued MAX_FRAMES_IN_FLIGHT frames ago.
    flushDeletionQueue(&ld->deletionQueue, ld->device, ld->allocator);
    advanceDeletionQueue(&ld->deletionQueue);

    updateSSBODescriptorSets(ld->device, particleGraphics, dm->currentFrame, particleGraphics->maxFrames);

    uint32_t imageIndex       = dm->currentFrame;
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

    vkResetFences(device, 1, &gdp->inFlightFence[dm->currentFrame]);

    if (nextImageResult != VK_SUCCESS)
    {
        exit(1);
    }

    // --- Render graph command recording ---
    {
        VkCommandBufferBeginInfo beginInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        };
        vkBeginCommandBuffer(gdp->commandBuffer[dm->currentFrame], &beginInfo);

        // Compute→graphics ownership transfer for particle SSBO
        VkBufferMemoryBarrier acquireBarrier = {
            .sType               = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
            .srcAccessMask       = 0,
            .dstAccessMask       = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,
            .srcQueueFamilyIndex = pd->queueFamilyIndices.computeFamily,
            .dstQueueFamilyIndex = pd->queueFamilyIndices.graphicsFamily,
            .buffer              = particleGraphics->computeP->deviceBuffer->ssbos->buffers[dm->currentFrame],
            .offset              = 0,
            .size                = VK_WHOLE_SIZE,
        };
        vkCmdPipelineBarrier(gdp->commandBuffer[dm->currentFrame],
                             VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                             VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
                             0, 0, NULL, 1, &acquireBarrier, 0, NULL);

        // Update swapchain image for this frame's color resource (index 0)
        rg->resources[0].image     = gdp->swapchain->p_scImages[dm->currentFrame];
        rg->resources[0].imageView = gdp->swapchain->p_scImageView[dm->currentFrame];

        rgExecute(rg, gdp->commandBuffer[dm->currentFrame], dm->currentFrame);

        // Transition swapchain image to present layout
        transitionImageLayout(NULL, VK_NULL_HANDLE,
                              gdp->swapchain->p_scImages[dm->currentFrame],
                              gdp->swapchain->currentSurfaceFormat->format,
                              VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                              VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, 1,
                              gdp->commandBuffer[dm->currentFrame]);

        vkEndCommandBuffer(gdp->commandBuffer[dm->currentFrame]);
    }

    MODEL_VIEW_PROJECTION* uboModels =
        updateModelUniformBuffers(dm->graphicPipeline->swapchain, deltaTime, camera, dm->graphicPipeline[0].scene);
    memcpy(gdp->deviceBuffer->pUbo->uniformBuffersMapped[dm->currentFrame], uboModels,
           sizeof(MODEL_VIEW_PROJECTION) * dm->graphicPipeline[0].scene->modelsManager.modelInstanceLen);

    // When no models are alive, the particle shader still reads binding 0 entry 0 for
    // camera view/proj.  Write an identity-model + current-camera MVP so particles
    // always have fresh data regardless of model visibility.
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
    // printLightUniformBufferData(lightGraphics->deviceBuffer->pUbo->uniformBuffersMapped, dm->currentFrame,
    //                             dm->graphicPipeline[2].scene->lightManager.lightInstanceCount);

    VkSemaphore waitSemaphores[] = {gdp->imgViewSemaphore[dm->currentFrame],
                                    particleGraphics->computeP->computeEndSemaphore[dm->currentFrame]};

    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
                                         VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

    VkSemaphore signalSemaphores[] = {gdp->renderEndFlag[dm->currentFrame]};

    VkSubmitInfo graphicQueuesubmitInfo = {
        .sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount   = 2,
        .pWaitSemaphores      = waitSemaphores,
        .pWaitDstStageMask    = waitStages,
        .commandBufferCount   = 1,
        .pCommandBuffers      = &(gdp->commandBuffer[dm->currentFrame]),
        .signalSemaphoreCount = 1,
        .pSignalSemaphores    = signalSemaphores,
    };

    if (vkQueueSubmit(*ld->queueManager.graphicsQueue, 1, &graphicQueuesubmitInfo,
                      gdp->inFlightFence[dm->currentFrame]) != VK_SUCCESS)
    {
    }
    else
    {
    }

    VkSwapchainKHR swapChains[] = {dm->graphicPipeline->swapchain->VkSwapchain};

    VkPresentInfoKHR presentInfo = {
        .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores    = signalSemaphores,
        .swapchainCount     = 1,
        .pSwapchains        = swapChains,
        .pImageIndices      = &aquireImageIndex,
        .pResults           = NULL,
    };

    VkResult presentResult = vkQueuePresentKHR(*ld->queueManager.presentQueue, &presentInfo);

    dm->currentFrame = (dm->currentFrame + 1) % gdp->maxFrames;

    /*void* mappedData;
    vkMapMemory(device, lightGraphics->deviceBuffer->ssbos->readBackBufferMemory, 0, VK_WHOLE_SIZE, 0,
    &mappedData);

    // 2. Cast to your expected type
    ReadBackBuffer* results       = (ReadBackBuffer*) (mappedData);
    uint8_t         printReadBack = 0;
    /*if (printReadBack)
        for (int index = 0; index < lightGraphics->deviceBuffer->ssbos->readBackBufferSize; index++)
        {
            if (results[index].pixel.r || (index % 10000 == 0))

            if (results[index].pixel.g || (index % 10000 == 0))

            if (results[index].pixel.b || (index % 10000 == 0))

            if (results[index].pixel.a || (index % 10000 == 0))

            if (results[index].pixel1.r || (index % 10000 == 0))

            if (results[index].pixel1.g || (index % 10000 == 0))

            if (results[index].pixel1.b || (index % 10000 == 0))

            if (results[index].pixel1.a || (index % 10000 == 0))

            if (results[index].pixel2.r || (index % 10000 == 0))

            if (results[index].pixel2.g || (index % 10000 == 0))

            if (results[index].pixel2.b || (index % 10000 == 0))

            if (results[index].pixel2.a || (index % 10000 == 0))

            if (results[index].pixel3.r || (index % 10000 == 0))

            if (results[index].pixel3.g || (index % 10000 == 0))

            if (results[index].pixel3.b || (index % 10000 == 0))

            if (results[index].pixel3.a || (index % 10000 == 0))
        }
    vkUnmapMemory(device, lightGraphics->deviceBuffer->ssbos->readBackBufferMemory);*/
}
