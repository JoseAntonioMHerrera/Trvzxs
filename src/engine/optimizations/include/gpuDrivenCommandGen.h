#pragma once
#include "mat4.h"
#include <vulkan/vulkan_core.h>

typedef enum RENDERABLE_OBJECTS
{
    MODEL = 0,
    LIGHT = 1,
} RENDERABLE_OBJECTS;

typedef struct RenderableObject
{
    vec4 boundingBox[2];

    RENDERABLE_OBJECTS objectType;
    uint32_t           objectIndex;

    mat4 modelMatrix;
} RenderableObject;

typedef struct UnifiedGPUDrawSystem
{

    VkBuffer       objectDataBuffer;
    VkDeviceMemory objectDataMemory;

    VkBuffer       modelDrawCommandsBuffer;
    VkBuffer       lightDrawCommandsBuffer;
    VkDeviceMemory drawCommandsMemory;

    VkBuffer       modelDrawCountBuffer;
    VkBuffer       lightDrawCountBuffer;
    VkDeviceMemory drawCountsMemory;

    uint32_t totalObjectCount;
    uint32_t modelCount;
    uint32_t lightCount;
} UnifiedGPUDrawSystem;
