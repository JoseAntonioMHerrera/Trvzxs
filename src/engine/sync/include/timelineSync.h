#pragma once
#include <vulkan/vulkan.h>
#include <stdint.h>

typedef struct FrameSync
{
    VkSemaphore graphicsTimeline;
    VkSemaphore computeTimeline;

    uint64_t graphicsValue;
    uint64_t computeValue;

    uint32_t maxFramesInFlight;
} FrameSync;

void createFrameSync(FrameSync* sync, VkDevice device, uint32_t maxFramesInFlight);
void destroyFrameSync(FrameSync* sync, VkDevice device);
