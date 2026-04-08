#pragma once
#include <vulkan/vulkan.h>
#include "vma_bridge.h"

typedef struct UniformBufferManager
{
    VkBuffer*      uniformBuffers;
    uint32_t       uniformBuffersLen;
    VmaAllocation* uniformAllocations;
    void**         uniformBuffersMapped;

    // Staging buffers for device-local UBOs/SSBOs that need per-frame CPU upload
    VkBuffer*      stagingBuffers;
    VmaAllocation* stagingAllocations;
    VkDeviceSize   stagingSize; // per-buffer size for vkCmdCopyBuffer
} UniformBufferManager;

typedef struct SSBOBufferManager
{
    VkBuffer*      buffers;
    uint32_t       buffersLen;
    VmaAllocation* allocations;
    void**         buffersMapped;

    VkBuffer      readbackBuffer;
    VmaAllocation readbackAlloc;
    size_t        readBackBufferSize;
} SSBOBufferManager;

typedef struct ReadBackBuffer
{
    GEOM_VERTEX_COLOR_RGBA pixel;
    GEOM_VERTEX_COLOR_RGBA pixel1;
    GEOM_VERTEX_COLOR_RGBA pixel2;
    GEOM_VERTEX_COLOR_RGBA pixel3;

} ReadBackBuffer;
