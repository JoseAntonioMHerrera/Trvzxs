#include <assert.h>
#include "displayManager.h"
#include "computePipeline.h"
#include "particle.h"

void cleanupSyncObjects(VkDevice device, GraphicPipelineToSurface* gdp){

    for (size_t i = 0; i < gdp->maxFrames; i++) {
        vkDestroySemaphore(device, gdp->renderEndFlag[i], NULL);
        vkDestroySemaphore(device, gdp->imgViewSemaphore[i], NULL);
        vkDestroyFence(device, gdp->inFlightFence[i], NULL);
    }

}


void createSDLSurface(DisplayManager *dm, VkInstance* instance, SDL_Window* window){
    SDL_bool result_surface_creation = SDL_Vulkan_CreateSurface(window,
                                  *instance,
                                  &dm->surface);
    printf("CREATE INSTANCE FLAG 5\n");
    assert(dm->surface);
    assert(*instance);

    assert(result_surface_creation == SDL_TRUE);
}


void firstFrameInitialPassCompute(PhysicalDevice *pd, LogicalDevice *ld, DisplayManager *dm,
                                GraphicPipelineToSurface *gdp, GraphicPipelineToSurface* particleGraphics,
                                VkDevice device) {
    uint32_t bufferIndex = (dm->currentFrame - 1 + particleGraphics->maxFrames) % particleGraphics->maxFrames;

    // Begin command buffer
    VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };

    // Begin recording
    if (vkBeginCommandBuffer(particleGraphics->computeP->commandBuffer[bufferIndex], &beginInfo) != VK_SUCCESS) {
        printf("Failed to begin recording command buffer!\n");
        return;
    }

    // End recording
    if (vkEndCommandBuffer(particleGraphics->computeP->commandBuffer[bufferIndex]) != VK_SUCCESS) {
        printf("Failed to end recording command buffer!\n");
        return;
    }

    // Submit info
    VkSubmitInfo initialSubmitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 0,
        .pWaitSemaphores = NULL,
        .pWaitDstStageMask = NULL,
        .commandBufferCount = 1,
        .pCommandBuffers = &particleGraphics->computeP->commandBuffer[bufferIndex],
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &particleGraphics->computeP->computeReadySemaphore[bufferIndex]
    };

    if (vkQueueSubmit(*ld->queueManager.computeQueue, 1, &initialSubmitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
        printf("Failed to submit initial command buffer!\n");
        return;
    }

    // Wait for this initial submission to complete
    vkQueueWaitIdle(*ld->queueManager.computeQueue);

    printf("END FIRSTFRAMEINITIALPASS\n");
}

void updateSSBODescriptorSets(
    VkDevice device,
    GraphicPipelineToSurface* particleGraphics,
    uint32_t currentFrame,
    uint32_t maxFrames
) {
    // Calculate previous frame index with wraparound
    uint32_t prevFrame = (currentFrame - 1 + maxFrames) % maxFrames;

    // Update SSBO descriptor bindings for previous and current frame
    VkDescriptorBufferInfo prevSSBOBufferInfo = {
        .buffer = particleGraphics->computeP->deviceBuffer->ssbos->buffers[prevFrame],
        .offset = 0,
        .range = sizeof(GEOM_PARTICLE) * 1024,
    };

    VkDescriptorBufferInfo currentSSBOBufferInfo = {
        .buffer = particleGraphics->computeP->deviceBuffer->ssbos->buffers[currentFrame],
        .offset = 0,
        .range = sizeof(GEOM_PARTICLE) * 1024,
    };

    // Create descriptor write updates for both SSBO bindings
    VkWriteDescriptorSet descriptorWrites[] = {
        {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = particleGraphics->descriptorSet[currentFrame],
            .dstBinding = 2,  // Previous frame SSBO binding
            .dstArrayElement = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = 1,
            .pBufferInfo = &prevSSBOBufferInfo,
            .pImageInfo = VK_NULL_HANDLE,
            .pTexelBufferView = VK_NULL_HANDLE,
        },
        {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = particleGraphics->descriptorSet[currentFrame],
            .dstBinding = 3,  // Current frame SSBO binding
            .dstArrayElement = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = 1,
            .pBufferInfo = &currentSSBOBufferInfo,
            .pImageInfo = VK_NULL_HANDLE,
            .pTexelBufferView = VK_NULL_HANDLE,
        },
    };

    // Update the descriptor sets
    vkUpdateDescriptorSets(device, 2, descriptorWrites, 0, VK_NULL_HANDLE);
}

void drawFrame(PhysicalDevice *pd, LogicalDevice *ld, DisplayManager *dm, GraphicPipelineToSurface *gdp, GraphicPipelineToSurface* particleGraphics,
VkDevice device, SDL_Window *window, float deltaTime) {
    printf("NEW DELTATIME %f", deltaTime);
    printf("MECAGOENTODO\n");
            printf("MECAGOENTODO\n");
    printf("SEMAPHORE PREV FRAME %d, con MAX FRAMES %d\n y CURRENT FRAME %d", (dm->currentFrame - 1 + particleGraphics->maxFrames) % particleGraphics->maxFrames, particleGraphics->maxFrames, dm->currentFrame);
    VkSemaphore signalComputeSemaphores[] = {particleGraphics->computeP->computeEndSemaphore[dm->currentFrame], particleGraphics->computeP->computeReadySemaphore[dm->currentFrame]};
    VkPipelineStageFlags waitDstFlagsSemph[] = {VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT};
    VkSemaphore waitSemaphoresCompute[] = {particleGraphics->computeP->computeReadySemaphore[(dm->currentFrame - 1 + particleGraphics->maxFrames) % particleGraphics->maxFrames]};

    vkResetFences(device, 1, &particleGraphics->computeP->computeInFlightFence[dm->currentFrame]);
            printf("ANTES DEL RECORD WACHIN\n");
    recordParticleCommandBuffer(particleGraphics->computeP, pd, ld, *particleGraphics->pipelineLayout, &particleGraphics->descriptorSet[dm->currentFrame], dm->currentFrame, deltaTime);
    VkCommandBuffer submitCommandBuffer[] = {(particleGraphics->computeP->commandBuffer[dm->currentFrame])};
    // COMPUTE SUBMIT WORKLOAD
            //printf("DRAW FRAME FLAG 1\n");
    VkSubmitInfo computeQueueSubmitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = waitSemaphoresCompute,
        .pWaitDstStageMask = waitDstFlagsSemph,
        .commandBufferCount = 1,
        .pCommandBuffers = submitCommandBuffer,
        .signalSemaphoreCount = 2,
        .pSignalSemaphores = signalComputeSemaphores,
        .pNext = NULL,
    };
                printf("DESPUES DEL RECORD WACHITURRO DEL RECORD WACHIN\n");
printf("Wait semaphore: %p\n", computeQueueSubmitInfo.pWaitSemaphores);
printf("Command buffer: %p\n", computeQueueSubmitInfo.pCommandBuffers);
printf("Fence: %p\n", particleGraphics->computeP->computeInFlightFence[dm->currentFrame]);
                    printf("DESPUES DEL end singletimecommands\n");

    if (vkQueueSubmit(*ld->queueManager.computeQueue, 1, &computeQueueSubmitInfo, particleGraphics->computeP->computeInFlightFence[dm->currentFrame]) != VK_SUCCESS) {
        //printf("failed to submit draw command buffer!");
    }
                    printf("DESPUES DEL submit\n");
    printf("VAMOS QUE NOS VAMOS EUGENIO\n");
    vkWaitForFences(device, 1, &particleGraphics->computeP->computeInFlightFence[dm->currentFrame], VK_TRUE, UINT64_MAX);
        printf("MECAGOENTODO\n");
    //printf("DRAW FRAME: frame %d: Fence %p\n", dm->currentFrame, gdp->inFlightFence[dm->currentFrame]);
    //printf("END DRAW FRAME LOG\n");
    vkWaitForFences(device, 1, &gdp->inFlightFence[dm->currentFrame], VK_TRUE, UINT64_MAX);
        updateSSBODescriptorSets(ld->device, particleGraphics, dm->currentFrame, particleGraphics->maxFrames);
    //printf("DRAW FRAME FLAG 1\n");
    //printf("LETS DRAW FRAME %d\n", dm->currentFrame);
    uint32_t imageIndex = dm->currentFrame;
    //printf("IMAGE INDEX VALE %u", imageIndex);
    uint32_t aquireImageIndex = dm->currentFrame;
    VkResult nextImageResult = vkAcquireNextImageKHR(device, dm->graphicPipeline->swapchain->VkSwapchain, UINT64_MAX,
    gdp->imgViewSemaphore[dm->currentFrame], VK_NULL_HANDLE, &aquireImageIndex);

    switch(nextImageResult){
        case VK_SUCCESS:
            break;
        case VK_ERROR_OUT_OF_DATE_KHR:
            printf("QUE ESTA PASANDO AQUI\n");
            printf("\n\n\n");
            recreateSwapChain(pd, ld, window);
            return;
        default:
            printf("Error a la hora de leer la siguiente imagen del dispositivo");
            exit(1);
    }
    //printf("BEFORE DRAW FRAME RESET FENCE con INDEX %u\n", dm->currentFrame);
    vkResetFences(device, 1, &gdp->inFlightFence[dm->currentFrame]);
 //printf("DRAW FRAME FLAG 1\n");
    if( nextImageResult != VK_SUCCESS){

        //printf("FALLA A LA HORA DE ADQUIRIR LA SIGUIENTE IMAGEN\n");
        exit(1);
    }
//printf("DRAW FRAME FLAG 1\n");
    recordGraphicsCommandBuffer(gdp, particleGraphics, dm, dm->currentFrame, pd);
    // 4. End Command Buffer Recording
    if (vkEndCommandBuffer(gdp->commandBuffer[dm->currentFrame]) != VK_SUCCESS) {
        printf("failed to record command buffer!");
    }
    // GRAPHICS QUEUE SUBMIT INFO
    printf("HEMOS RETORNADO DE GRAPHIOS COMMAND RECORDING AND RENDER PASS\n");
    MODEL_VIEW_PROJECTION ubo = updateUniformBuffers(dm->graphicPipeline->swapchain, deltaTime);
        printf("HEMOS RETORNADO DEL UPTADE THE UNIFORM BUFFER PROJECTION\n");
    VkSemaphore waitSemaphores[] = {gdp->imgViewSemaphore[dm->currentFrame], particleGraphics->computeP->computeEndSemaphore[dm->currentFrame]};
            printf("WAIT SEMAPHORES\n");
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
                printf("WAIT STAGES\n");
    VkSemaphore signalSemaphores[] = {gdp->renderEndFlag[dm->currentFrame]}; // Hay que comprobar si estoy sincronizando bien particulas compute y graphic,
                printf("SIGNAL SEMAPHORES\n");
//printf("DRAW FRAME FLAG 1\n");
    VkSubmitInfo graphicQueuesubmitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 2,
        .pWaitSemaphores = waitSemaphores,
        .pWaitDstStageMask = waitStages,
        .commandBufferCount = 1,
        .pCommandBuffers = &(gdp->commandBuffer[dm->currentFrame]),
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = signalSemaphores,
    };
    printf("HEMOS PASADO graphicsQUEUESUBMITINFO\n");

    memcpy(gdp->deviceBuffer->ubo->uniformBuffersMapped[dm->currentFrame], &ubo, sizeof(ubo));
    //ComputePipeline* computeP, LogicalDevice* ld, VkPipelineLayout layout, VkDescriptorSet descriptorSet, uint32_t imageIndex, float deltaTime
printf("HEMOS PASADO EL MEMCPY DEL UBO MAPPED TO GPU\n");
    // GRAPHIC SUBMIT WORKLOAD
    if (vkQueueSubmit(*ld->queueManager.graphicsQueue, 1, &graphicQueuesubmitInfo, gdp->inFlightFence[dm->currentFrame]) != VK_SUCCESS) {
        //printf("failed to submit draw command buffer!");
    }

    VkSwapchainKHR swapChains[] = {dm->graphicPipeline->swapchain->VkSwapchain};
//printf("DRAW FRAME FLAG 1\n");
    VkPresentInfoKHR presentInfo = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = signalSemaphores,
        .swapchainCount = 1,
        .pSwapchains = swapChains,
        .pImageIndices = &aquireImageIndex,
        .pResults = NULL,
    };
//printf("DRAW FRAME FLAG 1\n");
    vkQueuePresentKHR(*ld->queueManager.presentQueue, &presentInfo);
    //printf("DM CURRENT FRAME %d \n", dm->currentFrame);
        //printf("DM MAX FRAME %d \n", gdp->maxFrames);

    dm->currentFrame = (dm->currentFrame + 1) % gdp->maxFrames;



        //printf("NUEVO FRAME %d\n", dm->currentFrame);
            //printf("NUEVO FRAME %d\n", dm->currentFrame);
};
