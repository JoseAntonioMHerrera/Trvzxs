#pragma once

#include <vulkan/vulkan.h>
#include <stddef.h>
#include <stdio.h>
#include "transformations.h"
#include "vertex.h"
#include "compilerMacros.h"
#include "numberTypes.h"
#include "miscTypes.h"
#include "deviceManagment.h"
#include "commandBuffer.h"
#include <string.h>

typedef struct CommandPools CommandPools;

typedef struct DeviceBufferManager {
    VkBuffer *buffer;
    VkDeviceMemory *bufferMemory;
    size_t bufferLen;

    // Index Buffer
    VkBuffer *indexBuffer;
    VkDeviceMemory *indexBufferMemory;
    size_t indexLen;

    // Staging Buffer
    size_t stagingLen;
    VkBuffer *stagingBuffer;
    VkDeviceMemory *stagingBufferMemory;

    //UniformBuffers
    UniformBufferManager *ubo;

    //SSBO prefered use
    SSBOBufferManager *ssbos;

} DeviceBufferManager;

void createVkBuffer(PhysicalDevice* pd, VkDevice d, VkDeviceSize size, VkBufferUsageFlags usage, VkSharingMode sharingMode, VkMemoryPropertyFlags properties,
                    VkBuffer* buffer, VkDeviceMemory* bufferMemory);

void createIndiceBuffer(PhysicalDevice* pd, LogicalDevice* ld, DeviceBufferManager *dbm, SCENE *objects,
                                      CommandPools* commandPool);

void createGeometryBuffer(PhysicalDevice *pd, LogicalDevice *ld, DeviceBufferManager *dbm, SCENE *objects,
                                        CommandPools* commandPool);
