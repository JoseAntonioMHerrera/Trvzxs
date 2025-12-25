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
    printf("BUFFFER INDEX: %d\n", bufferIndex);
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

void printLightUniformBufferData(void** uniformBuffersMapped, int currentFrame, int lightInstanceCount)
{
    // Cast the void pointer to LightTransformations array
    LightTransformations* lightData = (LightTransformations*) uniformBuffersMapped[currentFrame];

    printf("=== Light Uniform Buffer Data (Frame %d) ===\n", currentFrame);
    printf("Light Instance Count: %d\n\n", lightInstanceCount);

    for (int i = 0; i < lightInstanceCount; i++)
    {
        printf("--- Light Instance %d ---\n", i);

        // Print MVP matrices
        printf("Model Matrix:\n");
        printMat4(lightData[i].mvp.model);

        printf("View Matrix:\n");
        printMat4(lightData[i].mvp.view);

        printf("Projection Matrix:\n");
        printMat4(lightData[i].mvp.proj);

        printf("Inverse View-Projection Matrix:\n");
        printMat4(lightData[i].mInverseVP);

        // Print camera data
        printf("Camera Data:\n");
        printCamera3D(lightData[i].camera);

        printf("\n");
    }
}

// Helper function to print a 4x4 matrix
void printMat4(mat4 matrix)
{
    for (int row = 0; row < 4; row++)
    {
        printf("  [");
        for (int col = 0; col < 4; col++)
        {
            printf("%8.3f", matrix[row][col]); // Adjust if your mat4 uses different indexing
            if (col < 3)
                printf(", ");
        }
        printf("]\n");
    }
    printf("\n");
}

// Helper function to print Camera3D
void printCamera3D(Camera3D camera)
{
    printf("  Screen Size: (%.3f, %.3f)\n", camera.screenSize[0], camera.screenSize[1]);
    printf("  Eye: (%.3f, %.3f, %.3f)\n", camera.eye[0], camera.eye[1], camera.eye[2]);
    printf("  Center: (%.3f, %.3f, %.3f)\n", camera.center[0], camera.center[1], camera.center[2]);
    printf("  up: (%.3f, %.3f, %.3f)\n", camera.up[0], camera.up[1], camera.up[2]);
}

void drawFrame(PhysicalDevice* pd, LogicalDevice* ld, DisplayManager* dm, GraphicPipelineToSurface* gdp,
               GraphicPipelineToSurface* particleGraphics, GraphicPipelineToSurface* lightGraphics, VkDevice device,
               SDL_Window* window, float deltaTime, Camera3D camera)
{
    // printf("=== DRAW FRAME START ===\n");
    // printf("Current frame: %d, Delta time: %.4f\n", dm->currentFrame, deltaTime);

    VkSemaphore          signalComputeSemaphores[] = {particleGraphics->computeP->computeEndSemaphore[dm->currentFrame],
                                                      particleGraphics->computeP->computeReadySemaphore[dm->currentFrame]};
    VkPipelineStageFlags waitDstFlagsSemph[]       = {VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT};
    VkSemaphore          waitSemaphoresCompute[]   = {
        particleGraphics->computeP->computeReadySemaphore[(dm->currentFrame - 1 + particleGraphics->maxFrames) %
                                                          particleGraphics->maxFrames]};

    // printf("Resetting compute fence for frame %d\n", dm->currentFrame);
    vkResetFences(device, 1, &particleGraphics->computeP->computeInFlightFence[dm->currentFrame]);

    // printf("Recording particle compute command buffer\n");
    recordParticleCommandBuffer(particleGraphics, pd, ld, *particleGraphics->pipelineLayout,
                                &particleGraphics->descriptorSet[dm->currentFrame], dm->currentFrame, deltaTime);
    VkCommandBuffer submitCommandBuffer[] = {(particleGraphics->computeP->commandBuffer[dm->currentFrame])};
    // COMPUTE SUBMIT WORKLOAD
    // printf("Submitting compute queue workload\n");
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
        // printf("Compute queue submitted successfully\n");
    }

    // printf("Waiting for compute fence completion\n");
    vkWaitForFences(device, 1, &particleGraphics->computeP->computeInFlightFence[dm->currentFrame], VK_TRUE,
                    UINT64_MAX);
    // printf("Compute fence completed\n");

    // printf("Waiting for graphics fence for frame %d\n", dm->currentFrame);
    vkWaitForFences(device, 1, &gdp->inFlightFence[dm->currentFrame], VK_TRUE, UINT64_MAX);
    // printf("Graphics fence completed\n");

    // printf("Updating SSBO descriptor sets\n");
    updateSSBODescriptorSets(ld->device, particleGraphics, dm->currentFrame, particleGraphics->maxFrames);

    uint32_t imageIndex       = dm->currentFrame;
    uint32_t aquireImageIndex = dm->currentFrame;
    // printf("Acquiring next swapchain image (index: %u)\n", aquireImageIndex);

    VkResult nextImageResult =
        vkAcquireNextImageKHR(device, dm->graphicPipeline->swapchain->VkSwapchain, UINT64_MAX,
                              gdp->imgViewSemaphore[dm->currentFrame], VK_NULL_HANDLE, &aquireImageIndex);

    // printf("Swapchain image acquisition result: %d, acquired index: %u\n", nextImageResult, aquireImageIndex);

    switch (nextImageResult)
    {
    case VK_SUCCESS:
        // printf("Swapchain image acquired successfully\n");
        break;
    case VK_ERROR_OUT_OF_DATE_KHR:
        printf("Swapchain out of date - recreating\n");
        recreateSwapChain(pd, ld, window);
        return;
    default:
        printf("ERROR: Failed to acquire next swapchain image, result: %d\n", nextImageResult);
        exit(1);
    }

    // printf("Resetting graphics fence for frame %d\n", dm->currentFrame);
    vkResetFences(device, 1, &gdp->inFlightFence[dm->currentFrame]);

    if (nextImageResult != VK_SUCCESS)
    {
        printf("ERROR: Next image result not successful: %d\n", nextImageResult);
        exit(1);
    }

    // printf("Recording graphics command buffer\n");
    recordGraphicsCommandBuffer(gdp, particleGraphics, lightGraphics, dm, dm->currentFrame, pd);

    // printf("Ending command buffer recording\n");
    if (vkEndCommandBuffer(gdp->commandBuffer[dm->currentFrame]) != VK_SUCCESS)
    {
        printf("ERROR: Failed to record command buffer!\n");
    }
    else
    {
        // printf("Command buffer recorded successfully\n");
    }

    // printf("Updating uniform buffers\n");
    MODEL_VIEW_PROJECTION* uboModels =
        updateModelUniformBuffers(dm->graphicPipeline->swapchain, deltaTime, camera, dm->graphicPipeline[0].scene);
    // printf("Instance 0 translation: (%.2f, %.2f, %.2f)\n", uboModels[0].model[3][0], uboModels[0].model[3][1],
    //  uboModels[0].model[3][2]);
    // printf("Instance 1 translation: (%.2f, %.2f, %.2f)\n", uboModels[1].model[3][0], uboModels[1].model[3][1],
    //     uboModels[1].model[3][2]);
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(pd->gpu[0], &props);
    // printf("minUniformBufferOffsetAlignment: %zu\n", props.limits.minUniformBufferOffsetAlignment);
    // printf("sizeof(MODEL_VIEW_PROJECTION): %zu\n", sizeof(MODEL_VIEW_PROJECTION));
    // printf("Copying model uniform buffer data to GPU (size: %zu bytes)\n",
    // sizeof(*uboModels) * dm->graphicPipeline[0].scene->modelsManager.modelInstanceLen);
    memcpy(gdp->deviceBuffer->pUbo->uniformBuffersMapped[dm->currentFrame], uboModels,
           sizeof(MODEL_VIEW_PROJECTION) * dm->graphicPipeline[0].scene->modelsManager.modelInstanceLen);

    LightTransformations* uboLights =
        updateLightUniformBuffers(dm->graphicPipeline->swapchain, deltaTime, camera, dm->graphicPipeline[2].scene);
    // printf("Copying light uniform buffer memory to GPU (size: %p bytes)\n",
    //        (size_t) lightGraphics->deviceBuffer->pUbo->uniformBuffersMemory);
    //  printf("Copying light uniform buffer data to GPU (size: %zu bytes)\n",
    // sizeof(*uboLights) * dm->graphicPipeline[2].scene->lightManager.lightInstanceCount);
    memcpy(lightGraphics->deviceBuffer->pUbo->uniformBuffersMapped[dm->currentFrame], uboLights,
           sizeof(*uboLights) * dm->graphicPipeline[2].scene->lightManager.lightInstanceCount);
    // printLightUniformBufferData(lightGraphics->deviceBuffer->pUbo->uniformBuffersMapped, dm->currentFrame,
    //                             dm->graphicPipeline[2].scene->lightManager.lightInstanceCount);

    VkSemaphore waitSemaphores[] = {gdp->imgViewSemaphore[dm->currentFrame],
                                    particleGraphics->computeP->computeEndSemaphore[dm->currentFrame]};

    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
                                         VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

    VkSemaphore signalSemaphores[] = {gdp->renderEndFlag[dm->currentFrame]};

    // printf("Submitting graphics queue workload\n");
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
        //      printf("Graphics queue submitted successfully\n");
    }

    VkSwapchainKHR swapChains[] = {dm->graphicPipeline->swapchain->VkSwapchain};

    //  printf("Presenting frame to swapchain\n");
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
    //  printf("Present result: %d\n", presentResult);

    ///    printf("Frame %d completed, advancing to next frame\n", dm->currentFrame);
    dm->currentFrame = (dm->currentFrame + 1) % gdp->maxFrames;
    //  printf("New current frame: %d (max frames: %d)\n", dm->currentFrame, gdp->maxFrames);
    // printf("=== DRAW FRAME END ===\n\n");

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
                printf("LightPixel %d pixel.r: %f\n", index, results[index].pixel.r);

            if (results[index].pixel.g || (index % 10000 == 0))
                printf("LightPixel %d pixel.g: %f\n", index, results[index].pixel.g);

            if (results[index].pixel.b || (index % 10000 == 0))
                printf("LightPixel %d pixel.b: %f\n", index, results[index].pixel.b);

            if (results[index].pixel.a || (index % 10000 == 0))
                printf("LightPixel %d pixel.a: %f\n", index, results[index].pixel.a);

            if (results[index].pixel1.r || (index % 10000 == 0))
                printf("LightPixel %d pixel1.r: %f\n", index, results[index].pixel1.r);

            if (results[index].pixel1.g || (index % 10000 == 0))
                printf("LightPixel %d pixel1.g: %f\n", index, results[index].pixel1.g);

            if (results[index].pixel1.b || (index % 10000 == 0))
                printf("LightPixel %d pixel1.b: %f\n", index, results[index].pixel1.b);

            if (results[index].pixel1.a || (index % 10000 == 0))
                printf("LightPixel %d pixel1.a: %f\n", index, results[index].pixel1.a);

            if (results[index].pixel2.r || (index % 10000 == 0))
                printf("LightPixel %d pixel2.r: %f\n", index, results[index].pixel2.r);

            if (results[index].pixel2.g || (index % 10000 == 0))
                printf("LightPixel %d pixel2.g: %f\n", index, results[index].pixel2.g);

            if (results[index].pixel2.b || (index % 10000 == 0))
                printf("LightPixel %d pixel2.b: %f\n", index, results[index].pixel2.b);

            if (results[index].pixel2.a || (index % 10000 == 0))
                printf("LightPixel %d pixel2.a: %f\n", index, results[index].pixel2.a);

            if (results[index].pixel3.r || (index % 10000 == 0))
                printf("LightPixel %d pixel3.r: %f\n", index, results[index].pixel3.r);

            if (results[index].pixel3.g || (index % 10000 == 0))
                printf("LightPixel %d pixel3.g: %f\n", index, results[index].pixel3.g);

            if (results[index].pixel3.b || (index % 10000 == 0))
                printf("LightPixel %d pixel3.b: %f\n", index, results[index].pixel3.b);

            if (results[index].pixel3.a || (index % 10000 == 0))
                printf("LightPixel %d pixel3.a: %f\n", index, results[index].pixel3.a);
        }
    vkUnmapMemory(device, lightGraphics->deviceBuffer->ssbos->readBackBufferMemory);*/
}
