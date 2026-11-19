#include "timelineSync.h"
#include <assert.h>
#include <stdlib.h>

void createFrameSync(FrameSync* sync, VkDevice device, uint32_t maxFramesInFlight)
{
    sync->graphicsWorkloadRun     = 0;
    sync->computeWorkloadRun      = 0;
    sync->maxFramesInFlight = maxFramesInFlight;
    sync->imageAvailableSemaphore = NULL;
    sync->renderEndFlag           = NULL;
    sync->renderEndFlagLen        = 0;

    VkSemaphoreTypeCreateInfo timelineInfo = {
        .sType         = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
        .semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
        .initialValue  = 0,
    };

    VkSemaphoreCreateInfo semaphoreInfo = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = &timelineInfo,
    };

    VkResult result = vkCreateSemaphore(device, &semaphoreInfo, NULL, &sync->graphicsTimeline);
    assert(result == VK_SUCCESS);

    result = vkCreateSemaphore(device, &semaphoreInfo, NULL, &sync->computeTimeline);
    assert(result == VK_SUCCESS);
}

void destroyFrameSync(FrameSync* sync, VkDevice device)
{
    destroySwapchainSemaphores(sync, device);

    vkDestroySemaphore(device, sync->graphicsTimeline, NULL);
    vkDestroySemaphore(device, sync->computeTimeline, NULL);

    sync->graphicsTimeline = VK_NULL_HANDLE;
    sync->computeTimeline  = VK_NULL_HANDLE;
    sync->graphicsWorkloadRun    = 0;
    sync->computeWorkloadRun     = 0;
}

void createSwapchainSemaphores(FrameSync* sync, VkDevice device, uint32_t swapchainImageCount)
{
    VkSemaphoreCreateInfo semaphoreInfo = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    };

    sync->imageAvailableSemaphore = malloc(sizeof(VkSemaphore) * sync->maxFramesInFlight);
    sync->renderEndFlag           = malloc(sizeof(VkSemaphore) * swapchainImageCount);
    sync->renderEndFlagLen        = swapchainImageCount;
    assert(sync->imageAvailableSemaphore);
    assert(sync->renderEndFlag);

    for (uint32_t i = 0; i < sync->maxFramesInFlight; i++)
    {
        VkResult result = vkCreateSemaphore(device, &semaphoreInfo, NULL, &sync->imageAvailableSemaphore[i]);
        assert(result == VK_SUCCESS);
    }

    for (uint32_t i = 0; i < swapchainImageCount; i++)
    {
        VkResult result = vkCreateSemaphore(device, &semaphoreInfo, NULL, &sync->renderEndFlag[i]);
        assert(result == VK_SUCCESS);
    }
}

void destroySwapchainSemaphores(FrameSync* sync, VkDevice device)
{
    if (sync->imageAvailableSemaphore)
    {
        for (uint32_t i = 0; i < sync->maxFramesInFlight; i++)
            vkDestroySemaphore(device, sync->imageAvailableSemaphore[i], NULL);
        free(sync->imageAvailableSemaphore);
        sync->imageAvailableSemaphore = NULL;
    }

    if (sync->renderEndFlag)
    {
        for (uint32_t i = 0; i < sync->renderEndFlagLen; i++)
            vkDestroySemaphore(device, sync->renderEndFlag[i], NULL);
        free(sync->renderEndFlag);
        sync->renderEndFlag    = NULL;
        sync->renderEndFlagLen = 0;
    }
}
