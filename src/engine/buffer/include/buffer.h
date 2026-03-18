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

typedef struct DeviceBufferManager
{
    VkBuffer*      buffer;
    VmaAllocation* bufferAlloc;
    size_t         bufferLen;

    // Index Buffer
    VkBuffer*      indexBuffer;
    VmaAllocation* indexBufferAlloc;
    size_t*        indexLen;

    // Staging Buffer
    size_t         stagingLen;
    VkBuffer*      stagingBuffer;
    VmaAllocation* stagingAlloc;

    // UniformBuffers
    UniformBufferManager* pUbo;
    uint32_t              uboCount;

    // SSBO prefered use
    SSBOBufferManager* ssbos;
    uint32_t           ssbosCount;
} DeviceBufferManager;

void createVkBuffer(VmaAllocator allocator, VkDeviceSize size, VkBufferUsageFlags usage,
                    VkSharingMode sharingMode, VmaBridgeMemoryUsage memUsage, VkBuffer* buffer,
                    VmaAllocation* allocation);

void createVkBufferMapped(VmaAllocator allocator, VkDeviceSize size, VkBufferUsageFlags usage,
                          VkSharingMode sharingMode, VkBuffer* buffer, VmaAllocation* allocation,
                          void** mappedPtr);

void createGeometryIndiceBuffer(PhysicalDevice* pd, LogicalDevice* ld, DeviceBufferManager* dbm, SCENE* scene,
                                CommandPools* commandPool);
void createParticleIndiceBuffers(PhysicalDevice* pd, LogicalDevice* ld, DeviceBufferManager* dbm, SCENE* scene,
                                 CommandPools* commandPool);
void createGeometryBuffer(PhysicalDevice* pd, LogicalDevice* ld, DeviceBufferManager* dbm, SCENE* scene,
                          CommandPools* commandPool);
void createLightVolumeIndiceBuffers(PhysicalDevice* pd, LogicalDevice* ld, DeviceBufferManager* dbm, SCENE* scene,
                                    CommandPools* commandPool);
void createLightVolumeVertexBuffer(PhysicalDevice* pd, LogicalDevice* ld, DeviceBufferManager* dbm, SCENE* scene,
                                   CommandPools* commandPool);
void createParticleBuffer(PhysicalDevice* pd, LogicalDevice* ld, DeviceBufferManager* dbm, SCENE* scene,
                          CommandPools* commandPool);
