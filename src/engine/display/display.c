#include "camera.h"
#include "computePipeline.h"
#include "displayManager.h"
#include "particle.h"
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
    printf("CREATE INSTANCE FLAG 5\n");
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
        printf("Failed to begin recording command buffer!\n");
        return;
    }

    // End recording
    if (vkEndCommandBuffer(particleGraphics->computeP->commandBuffer[bufferIndex]) != VK_SUCCESS)
    {
        printf("Failed to end recording command buffer!\n");
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
        printf("Failed to submit initial command buffer!\n");
        return;
    }

    // Wait for this initial submission to complete
    vkQueueWaitIdle(*ld->queueManager.computeQueue);

    printf("END FIRSTFRAMEINITIALPASS\n");
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
        .range  = sizeof(GEOM_PARTICLE) * 1024,
    };

    VkDescriptorBufferInfo currentSSBOBufferInfo = {
        .buffer = particleGraphics->computeP->deviceBuffer->ssbos->buffers[currentFrame],
        .offset = 0,
        .range  = sizeof(GEOM_PARTICLE) * 1024,
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
               SDL_Window* window, float deltaTime, Camera3D camera)
{
    printf("=== DRAW FRAME START ===\n");
    printf("Current frame: %d, Delta time: %.4f\n", dm->currentFrame, deltaTime);

    VkSemaphore          signalComputeSemaphores[] = {particleGraphics->computeP->computeEndSemaphore[dm->currentFrame],
                                                      particleGraphics->computeP->computeReadySemaphore[dm->currentFrame]};
    VkPipelineStageFlags waitDstFlagsSemph[]       = {VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT};
    VkSemaphore          waitSemaphoresCompute[]   = {
        particleGraphics->computeP->computeReadySemaphore[(dm->currentFrame - 1 + particleGraphics->maxFrames) %
                                                          particleGraphics->maxFrames]};

    printf("Resetting compute fence for frame %d\n", dm->currentFrame);
    vkResetFences(device, 1, &particleGraphics->computeP->computeInFlightFence[dm->currentFrame]);

    printf("Recording particle compute command buffer\n");
    recordParticleCommandBuffer(particleGraphics->computeP, pd, ld, *particleGraphics->pipelineLayout,
                                &particleGraphics->descriptorSet[dm->currentFrame], dm->currentFrame, deltaTime);
    VkCommandBuffer submitCommandBuffer[] = {(particleGraphics->computeP->commandBuffer[dm->currentFrame])};
    // COMPUTE SUBMIT WORKLOAD
    printf("Submitting compute queue workload\n");
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
        printf("ERROR: Failed to submit compute command buffer!\n");
    }
    else
    {
        printf("Compute queue submitted successfully\n");
    }

    printf("Waiting for compute fence completion\n");
    vkWaitForFences(device, 1, &particleGraphics->computeP->computeInFlightFence[dm->currentFrame], VK_TRUE,
                    UINT64_MAX);
    printf("Compute fence completed\n");

    printf("Waiting for graphics fence for frame %d\n", dm->currentFrame);
    vkWaitForFences(device, 1, &gdp->inFlightFence[dm->currentFrame], VK_TRUE, UINT64_MAX);
    printf("Graphics fence completed\n");

    printf("Updating SSBO descriptor sets\n");
    updateSSBODescriptorSets(ld->device, particleGraphics, dm->currentFrame, particleGraphics->maxFrames);

    uint32_t imageIndex       = dm->currentFrame;
    uint32_t aquireImageIndex = dm->currentFrame;
    printf("Acquiring next swapchain image (index: %u)\n", aquireImageIndex);

    VkResult nextImageResult =
        vkAcquireNextImageKHR(device, dm->graphicPipeline->swapchain->VkSwapchain, UINT64_MAX,
                              gdp->imgViewSemaphore[dm->currentFrame], VK_NULL_HANDLE, &aquireImageIndex);

    printf("Swapchain image acquisition result: %d, acquired index: %u\n", nextImageResult, aquireImageIndex);

    switch (nextImageResult)
    {
    case VK_SUCCESS:
        printf("Swapchain image acquired successfully\n");
        break;
    case VK_ERROR_OUT_OF_DATE_KHR:
        printf("Swapchain out of date - recreating\n");
        recreateSwapChain(pd, ld, window);
        return;
    default:
        printf("ERROR: Failed to acquire next swapchain image, result: %d\n", nextImageResult);
        exit(1);
    }

    printf("Resetting graphics fence for frame %d\n", dm->currentFrame);
    vkResetFences(device, 1, &gdp->inFlightFence[dm->currentFrame]);

    if (nextImageResult != VK_SUCCESS)
    {
        printf("ERROR: Next image result not successful: %d\n", nextImageResult);
        exit(1);
    }

    printf("Recording graphics command buffer\n");
    recordGraphicsCommandBuffer(gdp, particleGraphics, lightGraphics, dm, dm->currentFrame, pd);

    printf("Ending command buffer recording\n");
    if (vkEndCommandBuffer(gdp->commandBuffer[dm->currentFrame]) != VK_SUCCESS)
    {
        printf("ERROR: Failed to record command buffer!\n");
    }
    else
    {
        printf("Command buffer recorded successfully\n");
    }

    printf("Updating uniform buffers\n");
    MODEL_VIEW_PROJECTION* uboModels =
        updateModelUniformBuffers(dm->graphicPipeline->swapchain, deltaTime, camera, dm->graphicPipeline[0].scene);

    printf("Copying uniform buffer data to GPU (size: %zu bytes)\n",
           sizeof(uboModels) * dm->graphicPipeline[0].scene->modelsManager.modelInstanceLen);
    memcpy(gdp->deviceBuffer->pUbo->uniformBuffersMapped[dm->currentFrame], &uboModels,
           sizeof(uboModels) * dm->graphicPipeline[0].scene->modelsManager.modelInstanceLen);

    MODEL_VIEW_PROJECTION* uboLights =
        updateLightUniformBuffers(dm->graphicPipeline->swapchain, deltaTime, camera, dm->graphicPipeline[0].scene);
    printf("Copying uniform buffer data to GPU (size: %zu bytes)\n",
           sizeof(uboLights) * dm->graphicPipeline[2].scene->lightManager.lightInstanceCount);
    memcpy(lightGraphics->deviceBuffer->pUbo->uniformBuffersMapped[dm->currentFrame], &uboModels,
           sizeof(uboLights) * dm->graphicPipeline[2].scene->lightManager.lightInstanceCount);
    VkSemaphore waitSemaphores[] = {gdp->imgViewSemaphore[dm->currentFrame],
                                    particleGraphics->computeP->computeEndSemaphore[dm->currentFrame]};

    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
                                         VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

    VkSemaphore signalSemaphores[] = {gdp->renderEndFlag[dm->currentFrame]};

    printf("Submitting graphics queue workload\n");
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
        printf("ERROR: Failed to submit graphics command buffer!\n");
    }
    else
    {
        printf("Graphics queue submitted successfully\n");
    }

    VkSwapchainKHR swapChains[] = {dm->graphicPipeline->swapchain->VkSwapchain};

    printf("Presenting frame to swapchain\n");
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
    printf("Present result: %d\n", presentResult);

    printf("Frame %d completed, advancing to next frame\n", dm->currentFrame);
    dm->currentFrame = (dm->currentFrame + 1) % gdp->maxFrames;
    printf("New current frame: %d (max frames: %d)\n", dm->currentFrame, gdp->maxFrames);
    printf("=== DRAW FRAME END ===\n\n");
}
