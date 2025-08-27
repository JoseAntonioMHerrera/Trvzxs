#pragma once
#include <vulkan/vulkan.h>

typedef struct UniformBufferManager
{
    VkBuffer*       uniformBuffers;
    uint32_t        uniformBuffersLen;
    VkDeviceMemory* uniformBuffersMemory;
    void**          uniformBuffersMapped;
} UniformBufferManager;

typedef struct SSBOBufferManager
{
    VkBuffer*       buffers;
    uint32_t        buffersLen;
    VkDeviceMemory* buffersMemory;
    void**          buffersMapped;

    VkBuffer       readbackBuffer;
    VkDeviceMemory readBackBufferMemory;
    size_t         readBackBufferSize;
} SSBOBufferManager;

typedef struct ReadBackBuffer
{
    GEOM_VERTEX_COLOR_RGBA pixel;
    GEOM_VERTEX_COLOR_RGBA pixel1;
    GEOM_VERTEX_COLOR_RGBA pixel2;
    GEOM_VERTEX_COLOR_RGBA pixel3;

} ReadBackBuffer;
