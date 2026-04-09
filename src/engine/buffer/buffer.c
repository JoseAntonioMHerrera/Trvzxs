#include "buffer.h"
#include "primitives.h"
// TODO when the time comes, update the behaviour of staging buffers of buffer creation functions

void createVkBuffer(VmaAllocator allocator, VkDeviceSize size, VkBufferUsageFlags usage,
                    VkSharingMode sharingMode, VmaBridgeMemoryUsage memUsage, VkBuffer* buffer,
                    VmaAllocation* allocation)
{
    VkResult result = vmaBridgeCreateBuffer(allocator, size, usage, sharingMode, memUsage, buffer, allocation);
    if (result != VK_SUCCESS)
    {
        exit(1);
    }
}

void createVkBufferMapped(VmaAllocator allocator, VkDeviceSize size, VkBufferUsageFlags usage,
                          VkSharingMode sharingMode, VkBuffer* buffer, VmaAllocation* allocation,
                          void** mappedPtr)
{
    VkResult result = vmaBridgeCreateBufferMapped(allocator, size, usage, sharingMode, buffer, allocation, mappedPtr);
    if (result != VK_SUCCESS)
    {
        exit(1);
    }
}

void createGeometryIndiceBuffer(PhysicalDevice* pd, LogicalDevice* ld, DeviceBufferManager* dbm, SCENE* objects,
                                CommandPools* commandPool)
{

    VkDeviceSize bufferSize =
        sizeof(*objects->modelsManager.packedModelIndex.items) * objects->modelsManager.packedModelIndex.itemsLen;

    dbm->stagingBuffer = malloc(sizeof(*dbm->stagingBuffer));
    dbm->stagingAlloc  = malloc(sizeof(*dbm->stagingAlloc));

    createVkBuffer(ld->allocator, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_SHARING_MODE_EXCLUSIVE,
                   VMA_BRIDGE_MEMORY_CPU_TO_GPU, dbm->stagingBuffer, dbm->stagingAlloc);

    void* data;
    vmaBridgeMapMemory(ld->allocator, *dbm->stagingAlloc, &data);
    memcpy(data, objects->modelsManager.packedModelIndex.items, (VkDeviceSize) bufferSize);
    vmaBridgeUnmapMemory(ld->allocator, *dbm->stagingAlloc);

    dbm->indexBuffer      = malloc(sizeof(*dbm->indexBuffer));
    dbm->indexBufferAlloc = malloc(sizeof(*dbm->indexBufferAlloc));

    createVkBuffer(ld->allocator, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                   VK_SHARING_MODE_EXCLUSIVE, VMA_BRIDGE_MEMORY_GPU_ONLY, dbm->indexBuffer, dbm->indexBufferAlloc);

    copyBuffer(ld, *dbm->stagingBuffer, *dbm->indexBuffer, bufferSize, commandPool->transferCommandPool,
               *(ld->queueManager.transferQueue));

    vmaBridgeDestroyBuffer(ld->allocator, *dbm->stagingBuffer, *dbm->stagingAlloc);
}
void createParticleIndiceBuffers(PhysicalDevice* pd, LogicalDevice* ld, DeviceBufferManager* dbm, SCENE* scene,
                                 CommandPools* commandPool)
{

    VkDeviceSize particleIndicesBufferSize =
        sizeof(*(scene->particleManager.pPackedParticleIndices)) * scene->particleManager.packedParticleIndicesLen;

    dbm->stagingBuffer = malloc(sizeof(*dbm->stagingBuffer));
    dbm->stagingAlloc  = malloc(sizeof(*dbm->stagingAlloc));

    createVkBuffer(ld->allocator, particleIndicesBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                   VK_SHARING_MODE_EXCLUSIVE, VMA_BRIDGE_MEMORY_CPU_TO_GPU, dbm->stagingBuffer, dbm->stagingAlloc);

    void* memoryMappedData;
    vmaBridgeMapMemory(ld->allocator, *dbm->stagingAlloc, &memoryMappedData);
    memcpy(memoryMappedData, scene->particleManager.pPackedParticleIndices, (size_t) particleIndicesBufferSize);
    vmaBridgeUnmapMemory(ld->allocator, *dbm->stagingAlloc);

    dbm->indexBuffer      = malloc(sizeof(*dbm->indexBuffer));
    dbm->indexBufferAlloc = malloc(sizeof(*dbm->indexBufferAlloc));

    createVkBuffer(ld->allocator, particleIndicesBufferSize,
                   VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_SHARING_MODE_EXCLUSIVE,
                   VMA_BRIDGE_MEMORY_GPU_ONLY, dbm->indexBuffer, dbm->indexBufferAlloc);

    copyBuffer(ld, *dbm->stagingBuffer, *dbm->indexBuffer, particleIndicesBufferSize, commandPool->transferCommandPool,
               *(ld->queueManager.transferQueue));

    vmaBridgeDestroyBuffer(ld->allocator, *dbm->stagingBuffer, *dbm->stagingAlloc);
}
void createGeometryBuffer(PhysicalDevice* pd, LogicalDevice* ld, DeviceBufferManager* dbm, SCENE* scene,
                          CommandPools* commandPool)
{

    VkDeviceSize geometryBufferSize =
        sizeof(*(scene->modelsManager.pPackedModelVertexLean)) * scene->modelsManager.packedModelVertexLeanLen;

    dbm->stagingBuffer = malloc(sizeof(*dbm->stagingBuffer));
    dbm->stagingAlloc  = malloc(sizeof(*dbm->stagingAlloc));

    createVkBuffer(ld->allocator, geometryBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_SHARING_MODE_EXCLUSIVE,
                   VMA_BRIDGE_MEMORY_CPU_TO_GPU, dbm->stagingBuffer, dbm->stagingAlloc);

    void* memoryMappedData;
    vmaBridgeMapMemory(ld->allocator, *dbm->stagingAlloc, &memoryMappedData);
    memcpy(memoryMappedData, scene->modelsManager.pPackedModelVertexLean, (size_t) geometryBufferSize);
    vmaBridgeUnmapMemory(ld->allocator, *dbm->stagingAlloc);

    dbm->buffer      = malloc(sizeof(*dbm->buffer));
    dbm->bufferAlloc = malloc(sizeof(*dbm->bufferAlloc));

    createVkBuffer(ld->allocator, geometryBufferSize,
                   VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE,
                   VMA_BRIDGE_MEMORY_GPU_ONLY, dbm->buffer, dbm->bufferAlloc);

    copyBuffer(ld, *dbm->stagingBuffer, *dbm->buffer, geometryBufferSize, commandPool->transferCommandPool,
               *(ld->queueManager.transferQueue));

    vmaBridgeDestroyBuffer(ld->allocator, *dbm->stagingBuffer, *dbm->stagingAlloc);
}

void createLightVolumeVertexBuffer(PhysicalDevice* pd, LogicalDevice* ld, DeviceBufferManager* dbm, SCENE* scene,
                                   CommandPools* commandPool)
{

    VkDeviceSize lightsBufferSize = sizeof(*(scene->lightManager.pPackedLight)) * scene->lightManager.packedLightLen;

    dbm->stagingBuffer = malloc(sizeof(*dbm->stagingBuffer));
    dbm->stagingAlloc  = malloc(sizeof(*dbm->stagingAlloc));

    createVkBuffer(ld->allocator, lightsBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_SHARING_MODE_EXCLUSIVE,
                   VMA_BRIDGE_MEMORY_CPU_TO_GPU, dbm->stagingBuffer, dbm->stagingAlloc);

    void* memoryMappedData;
    vmaBridgeMapMemory(ld->allocator, *dbm->stagingAlloc, &memoryMappedData);
    memcpy(memoryMappedData, scene->lightManager.pPackedLight, (size_t) lightsBufferSize);
    vmaBridgeUnmapMemory(ld->allocator, *dbm->stagingAlloc);

    dbm->buffer      = malloc(sizeof(*dbm->buffer));
    dbm->bufferAlloc = malloc(sizeof(*dbm->bufferAlloc));

    createVkBuffer(ld->allocator, lightsBufferSize,
                   VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE,
                   VMA_BRIDGE_MEMORY_GPU_ONLY, dbm->buffer, dbm->bufferAlloc);

    copyBuffer(ld, *dbm->stagingBuffer, *dbm->buffer, lightsBufferSize, commandPool->transferCommandPool,
               *(ld->queueManager.transferQueue));

    vmaBridgeDestroyBuffer(ld->allocator, *dbm->stagingBuffer, *dbm->stagingAlloc);
}

void createLightVolumeIndiceBuffers(PhysicalDevice* pd, LogicalDevice* ld, DeviceBufferManager* dbm, SCENE* scene,
                                    CommandPools* commandPool)
{

    VkDeviceSize bufferSize =
        sizeof(*(scene->lightManager.packedLightIndex.items)) * scene->lightManager.packedLightIndex.itemsLen;

    dbm->stagingBuffer = malloc(sizeof(*dbm->stagingBuffer));
    dbm->stagingAlloc  = malloc(sizeof(*dbm->stagingAlloc));

    createVkBuffer(ld->allocator, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_SHARING_MODE_EXCLUSIVE,
                   VMA_BRIDGE_MEMORY_CPU_TO_GPU, dbm->stagingBuffer, dbm->stagingAlloc);

    void* data;
    vmaBridgeMapMemory(ld->allocator, *dbm->stagingAlloc, &data);
    memcpy(data, scene->lightManager.packedLightIndex.items, (size_t) bufferSize);
    vmaBridgeUnmapMemory(ld->allocator, *dbm->stagingAlloc);

    dbm->indexBuffer      = malloc(sizeof(*dbm->indexBuffer));
    dbm->indexBufferAlloc = malloc(sizeof(*dbm->indexBufferAlloc));

    createVkBuffer(ld->allocator, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                   VK_SHARING_MODE_EXCLUSIVE, VMA_BRIDGE_MEMORY_GPU_ONLY, dbm->indexBuffer, dbm->indexBufferAlloc);

    copyBuffer(ld, *dbm->stagingBuffer, *dbm->indexBuffer, bufferSize, commandPool->transferCommandPool,
               *(ld->queueManager.transferQueue));

    vmaBridgeDestroyBuffer(ld->allocator, *dbm->stagingBuffer, *dbm->stagingAlloc);
}

void createParticleBuffer(PhysicalDevice* pd, LogicalDevice* ld, DeviceBufferManager* dbm, SCENE* scene,
                          CommandPools* commandPool)
{

    VkDeviceSize particleBufferSize =
        sizeof(*(scene->particleManager.pPackedParticleVertex)) * scene->particleManager.packedParticleVertexLen;

    dbm->stagingBuffer = malloc(sizeof(*dbm->stagingBuffer));
    dbm->stagingAlloc  = malloc(sizeof(*dbm->stagingAlloc));

    createVkBuffer(ld->allocator, particleBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_SHARING_MODE_EXCLUSIVE,
                   VMA_BRIDGE_MEMORY_CPU_TO_GPU, dbm->stagingBuffer, dbm->stagingAlloc);

    void* memoryMappedData;
    vmaBridgeMapMemory(ld->allocator, *dbm->stagingAlloc, &memoryMappedData);
    printTrzVertex(scene->particleManager.pPackedParticleVertex, particleBufferSize / 64);
    memcpy(memoryMappedData, scene->particleManager.pPackedParticleVertex, (size_t) particleBufferSize);
    vmaBridgeUnmapMemory(ld->allocator, *dbm->stagingAlloc);

    dbm->buffer      = malloc(sizeof(*dbm->buffer));
    dbm->bufferAlloc = malloc(sizeof(*dbm->bufferAlloc));

    createVkBuffer(ld->allocator, particleBufferSize,
                   VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE,
                   VMA_BRIDGE_MEMORY_GPU_ONLY, dbm->buffer, dbm->bufferAlloc);

    copyBuffer(ld, *dbm->stagingBuffer, *dbm->buffer, particleBufferSize, commandPool->transferCommandPool,
               *(ld->queueManager.transferQueue));

    vmaBridgeDestroyBuffer(ld->allocator, *dbm->stagingBuffer, *dbm->stagingAlloc);
}
