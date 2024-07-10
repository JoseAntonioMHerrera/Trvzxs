#pragma once
#include <SDL2/SDL.h>
#include <stddef.h>
#include "deviceManagmentTypes.h"
#include "swapchainManager.h"
#include "graphicsPipeline.h"
#define MAX_FRAMES_IN_FLIGHT 2
TRVZXS_inline void  drawFrame(PhysicalDevice *pd, LogicalDevice *ld, DisplayManager *dm, GeometryDisplayPipelineManager *gdp,
VkDevice device, SDL_Window *window) {
    vkWaitForFences(device, 1, &gdp->inFlightFence[dm->currentFrame], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    VkResult nextImageResult = vkAcquireNextImageKHR(device, dm->swapChainM->swapchain, UINT64_MAX,
    gdp->imgViewSemaphore[dm->currentFrame], VK_NULL_HANDLE, &imageIndex);

    switch(nextImageResult){
        case VK_SUCCESS:
            break;
        case VK_ERROR_OUT_OF_DATE_KHR:
            recreateSwapChain(pd, ld, window);
            return;
        default:
            printf("Error a la hora de leer la siguiente imagen del dispositivo");
            exit(1);
    }
    vkResetFences(device, 1, &gdp->inFlightFence[dm->currentFrame]);

    if( nextImageResult != VK_SUCCESS){

        printf("FALLA A LA HORA DE ADQUIRIR LA SIGUIENTE IMAGEN\n");
        exit(1);
    }

    recordCommandBuffer(gdp, dm, imageIndex);
    // 4. End Command Buffer Recording
    if (vkEndCommandBuffer(gdp->commandBuffer[dm->currentFrame]) != VK_SUCCESS) {
        printf("failed to record command buffer!");
    }
    VkSemaphore waitSemaphores[] = {gdp->imgViewSemaphore[dm->currentFrame]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSemaphore signalSemaphores[] = {gdp->renderEndFlag[dm->currentFrame]};

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

    if (vkQueueSubmit(ld->queueManager.graphicsQueue, 1, &submitInfo, gdp->inFlightFence[dm->currentFrame]) != VK_SUCCESS) {
        printf("failed to submit draw command buffer!");
    }

    VkSwapchainKHR swapChains[] = {dm->swapChainM->swapchain};

    VkPresentInfoKHR presentInfo = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = signalSemaphores,
        .swapchainCount = 1,
        .pSwapchains = swapChains,
        .pImageIndices = &imageIndex,
        .pResults = NULL,
    };

    vkQueuePresentKHR(ld->queueManager.presentQueue, &presentInfo);

    dm->currentFrame = (dm->currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
};


TRVZXS_inline void cleanupSyncObjects(VkDevice device, GeometryDisplayPipelineManager* gdp){

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(device, gdp->renderEndFlag[i], NULL);
        vkDestroySemaphore(device, gdp->imgViewSemaphore[i], NULL);
        vkDestroyFence(device, gdp->inFlightFence[i], NULL);
    }

}
