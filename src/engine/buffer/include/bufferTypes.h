#pragma once
#include <vulkan/vulkan.h>

typedef struct UniformBufferManager {
    VkBuffer *uniformBuffers;
    uint32_t uniformBuffersLen;
    VkDeviceMemory *uniformBuffersMemory;
    void** uniformBuffersMapped;
} UniformBufferManager;


typedef struct SSBOBufferManager {
    VkBuffer *buffers;
    uint32_t buffersLen;
    VkDeviceMemory *buffersMemory;
    void** buffersMapped;

    VkBuffer readbackBuffer;
    VkDeviceMemory readBackBufferMemory;
} SSBOBufferManager;
