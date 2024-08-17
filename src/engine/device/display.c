#include <assert.h>
#include "displayManager.h"

void drawFrame(PhysicalDevice *pd, LogicalDevice *ld, DisplayManager *dm, GeometryDisplayPipelineManager *gdp,
VkDevice device, SDL_Window *window) {
    printf("DRAW FRAME: frame %d: Fence %p\n", dm->currentFrame, gdp->inFlightFence[dm->currentFrame]);
    printf("END DRAW FRAME LOG\n");
    vkWaitForFences(device, 1, &gdp->inFlightFence[dm->currentFrame], VK_TRUE, UINT64_MAX);
    printf("DRAW FRAME FLAG 1\n");
    printf("LETS DRAW FRAME %d\n", dm->currentFrame);
    uint32_t imageIndex = dm->currentFrame;
    printf("IMAGE INDEX VALE %u", imageIndex);
    uint32_t aquireImageIndex = dm->currentFrame;
    VkResult nextImageResult = vkAcquireNextImageKHR(device, dm->swapChainM->swapchain, UINT64_MAX,
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
    printf("BEFORE DRAW FRAME RESET FENCE con INDEX %u\n", dm->currentFrame);
    vkResetFences(device, 1, &gdp->inFlightFence[dm->currentFrame]);
printf("DRAW FRAME FLAG 1\n");
    if( nextImageResult != VK_SUCCESS){

        printf("FALLA A LA HORA DE ADQUIRIR LA SIGUIENTE IMAGEN\n");
        exit(1);
    }
printf("DRAW FRAME FLAG 1\n");
    recordCommandBuffer(gdp, dm, dm->currentFrame);
    // 4. End Command Buffer Recording
    if (vkEndCommandBuffer(gdp->commandBuffer[dm->currentFrame]) != VK_SUCCESS) {
        printf("failed to record command buffer!");
    }
    VkSemaphore waitSemaphores[] = {gdp->imgViewSemaphore[dm->currentFrame]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSemaphore signalSemaphores[] = {gdp->renderEndFlag[dm->currentFrame]};
printf("DRAW FRAME FLAG 1\n");
    VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = waitSemaphores,
        .pWaitDstStageMask = waitStages,
        .commandBufferCount = 1,
        .pCommandBuffers = &(gdp->commandBuffer[dm->currentFrame]),
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = signalSemaphores,
    };
printf("DRAW FRAME FLAG 1\n");
    MODEL_VIEW_PROJECTION ubo = updateUniformBuffers(dm->swapChainM);
    memcpy(gdp->deviceBuffer->ubo->uniformBuffersMapped[dm->currentFrame], &ubo, sizeof(ubo));
    if (vkQueueSubmit(*ld->queueManager.graphicsQueue, 1, &submitInfo, gdp->inFlightFence[dm->currentFrame]) != VK_SUCCESS) {
        printf("failed to submit draw command buffer!");
    }

    VkSwapchainKHR swapChains[] = {dm->swapChainM->swapchain};
printf("DRAW FRAME FLAG 1\n");
    VkPresentInfoKHR presentInfo = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = signalSemaphores,
        .swapchainCount = 1,
        .pSwapchains = swapChains,
        .pImageIndices = &aquireImageIndex,
        .pResults = NULL,
    };
printf("DRAW FRAME FLAG 1\n");
    vkQueuePresentKHR(*ld->queueManager.presentQueue, &presentInfo);
    printf("DM CURRENT FRAME %d \n", dm->currentFrame);
        printf("DM MAX FRAME %d \n", gdp->maxFrames);

    dm->currentFrame = (dm->currentFrame + 1) % gdp->maxFrames;



        printf("NUEVO FRAME %d\n", dm->currentFrame);
            printf("NUEVO FRAME %d\n", dm->currentFrame);
};
