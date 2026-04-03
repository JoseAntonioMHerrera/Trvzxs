#include "timelineSync.h"
#include <assert.h>

void createFrameSync(FrameSync* sync, VkDevice device, uint32_t maxFramesInFlight)
{
    sync->graphicsValue     = 0;
    sync->computeValue      = 0;
    sync->maxFramesInFlight = maxFramesInFlight;

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
    vkDestroySemaphore(device, sync->graphicsTimeline, NULL);
    vkDestroySemaphore(device, sync->computeTimeline, NULL);

    sync->graphicsTimeline = VK_NULL_HANDLE;
    sync->computeTimeline  = VK_NULL_HANDLE;
    sync->graphicsValue    = 0;
    sync->computeValue     = 0;
}
