#pragma once
#include <vulkan/vulkan.h>
#include <stdint.h>

typedef struct FrameSync
{
    VkSemaphore graphicsTimeline;
    VkSemaphore computeTimeline;

    uint64_t graphicsWorkloadRun;
    uint64_t computeWorkloadRun;

    uint32_t maxFramesInFlight;
} FrameSync;

void createFrameSync(FrameSync* sync, VkDevice device, uint32_t maxFramesInFlight);
void destroyFrameSync(FrameSync* sync, VkDevice device);
