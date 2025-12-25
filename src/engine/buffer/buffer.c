#include "buffer.h"
#include "primitives.h"
// TODO when the time comes, update the behaviour of staging buffers of buffer creation functions

void createVkBuffer(PhysicalDevice* pd, VkDevice d, VkDeviceSize size, VkBufferUsageFlags usage,
                    VkSharingMode sharingMode, VkMemoryPropertyFlags properties, VkBuffer* buffer,
                    VkDeviceMemory* bufferMemory)
{

    VkBufferCreateInfo bufferInfo = {
        .sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size        = size,
        .usage       = usage,
        .sharingMode = sharingMode,
        .flags       = 0,
        .pNext       = VK_NULL_HANDLE,
    };

    if (vkCreateBuffer(d, &bufferInfo, VK_NULL_HANDLE, buffer) != VK_SUCCESS)
    {
        printf("ERROR EN CREATEBUFFER\n");
        exit(1);
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(d, *buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {
        .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize  = memRequirements.size,
        .memoryTypeIndex = findMemoryType(pd, memRequirements.memoryTypeBits, properties),
        .pNext           = VK_NULL_HANDLE,

    };

    if (vkAllocateMemory(d, &allocInfo, VK_NULL_HANDLE, bufferMemory) != VK_SUCCESS)
    {

        exit(1);
    }

    VkResult result = vkBindBufferMemory(d, *buffer, *bufferMemory, 0);
    if (result != VK_SUCCESS)
    {
        printf("ERROR: vkBindBufferMemory failed with result: %d\n", result);
        exit(1);
    }
}

void createGeometryIndiceBuffer(PhysicalDevice* pd, LogicalDevice* ld, DeviceBufferManager* dbm, SCENE* objects,
                                CommandPools* commandPool)
{
    printf("=== CREATING GEOMETRY INDEX BUFFER ===\n");
    printf("Index count: %u\n", objects->modelsManager.packedModelIndex.itemsLen);

    VkDeviceSize bufferSize =
        sizeof(*objects->modelsManager.packedModelIndex.items) * objects->modelsManager.packedModelIndex.itemsLen;
    printf("Index buffer size calculation: sizeof(uint32_t)=%zu * indexCount=%u = %llu bytes (%.2f KB)\n",
           sizeof(uint32_t), objects->modelsManager.packedModelIndex.itemsLen, (unsigned long long) bufferSize,
           (double) bufferSize / 1024.0);

    dbm->stagingBuffer       = malloc(sizeof(*dbm->stagingBuffer));
    dbm->stagingBufferMemory = malloc(sizeof(*dbm->stagingBufferMemory));
    printf("Allocated staging buffer pointers: buffer=%p, memory=%p\n", dbm->stagingBuffer, dbm->stagingBufferMemory);
    printf("Creating staging buffer for indices with size %llu bytes\n", (unsigned long long) bufferSize);

    createVkBuffer(pd, ld->device, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_SHARING_MODE_EXCLUSIVE,
                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, dbm->stagingBuffer,
                   dbm->stagingBufferMemory);
    printf("Staging buffer created: handle=%p, memory=%p\n", (void*) (uintptr_t) *dbm->stagingBuffer,
           (void*) (uintptr_t) *dbm->stagingBufferMemory);

    void* data;
    printf("Mapping staging buffer memory for index data copy\n");
    vkMapMemory(ld->device, *dbm->stagingBufferMemory, 0, bufferSize, 0, &data);
    printf("Memory mapped at address: %p\n", data);
    printf("Source index data address: %p\n", objects->modelsManager.pPackedModelVertex);
    printf("WARNING: Copying vertex data to index buffer - this looks incorrect!\n");
    printf("Copying %llu bytes from vertex data to index staging buffer\n", (unsigned long long) bufferSize);

    memcpy(data, objects->modelsManager.packedModelIndex.items, (VkDeviceSize) bufferSize);
    printf("Index data copied to staging buffer\n");

    vkUnmapMemory(ld->device, *dbm->stagingBufferMemory);
    printf("Staging buffer memory unmapped\n");

    dbm->indexBuffer       = malloc(sizeof(*dbm->buffer));
    dbm->indexBufferMemory = malloc(sizeof(*dbm->buffer));
    printf("Allocated index buffer pointers: buffer=%p, memory=%p\n", dbm->indexBuffer, dbm->indexBufferMemory);
    printf("Creating device-local index buffer with size %llu bytes\n", (unsigned long long) bufferSize);

    createVkBuffer(pd, ld->device, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                   VK_SHARING_MODE_EXCLUSIVE, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, dbm->indexBuffer,
                   dbm->indexBufferMemory);
    printf("Index buffer created: handle=%p, memory=%p\n", (void*) (uintptr_t) *dbm->indexBuffer,
           (void*) (uintptr_t) *dbm->indexBufferMemory);

    printf("Copying data from staging buffer to index buffer (%llu bytes)\n", (unsigned long long) bufferSize);
    copyBuffer(ld, *dbm->stagingBuffer, *dbm->indexBuffer, bufferSize, commandPool->transferCommandPool,
               *(ld->queueManager.transferQueue));
    printf("Index buffer copy operation completed\n");

    printf("Cleaning up staging buffer resources\n");
    vkDestroyBuffer(ld->device, *dbm->stagingBuffer, VK_NULL_HANDLE);
    vkFreeMemory(ld->device, *dbm->stagingBufferMemory, VK_NULL_HANDLE);
    printf("Staging buffer destroyed and memory freed\n");
    printf("=== INDEX BUFFER CREATION COMPLETE ===\n");
}
void createParticleIndiceBuffers(PhysicalDevice* pd, LogicalDevice* ld, DeviceBufferManager* dbm, SCENE* scene,
                                 CommandPools* commandPool)
{
    printf("=== CREATING PARTICLE INDICE BUFFER ===\n");

    VkDeviceSize particleIndicesBufferSize =
        sizeof(*(scene->particleManager.pPackedParticleIndices)) * scene->particleManager.packedParticleIndicesLen;
    printf("Particle buffer size calculation: sizeof(vertex)=%zu * vertexCount=%d = %llu bytes (%.2f KB)\n",
           sizeof(*(scene->particleManager.pPackedParticleIndices)), scene->particleManager.packedParticleIndicesLen,
           (unsigned long long) particleIndicesBufferSize, (double) particleIndicesBufferSize / 1024.0);

    dbm->stagingBuffer       = malloc(sizeof(*dbm->stagingBuffer));
    dbm->stagingBufferMemory = malloc(sizeof(*dbm->stagingBufferMemory));
    printf("Allocated staging buffer pointers: buffer=%p, memory=%p\n", dbm->stagingBuffer, dbm->stagingBufferMemory);
    printf("Creating staging buffer with size %llu bytes\n", (unsigned long long) particleIndicesBufferSize);

    createVkBuffer(pd, ld->device, particleIndicesBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                   VK_SHARING_MODE_EXCLUSIVE,
                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, dbm->stagingBuffer,
                   dbm->stagingBufferMemory);
    printf("Staging buffer created: handle=%p, memory=%p\n", (void*) (uintptr_t) *dbm->stagingBuffer,
           (void*) (uintptr_t) *dbm->stagingBufferMemory);

    void* memoryMappedData; // Dst memory to copy from buffer data to gpu memory
    printf("Mapping staging buffer memory for vertex data copy\n");
    vkMapMemory(ld->device, *dbm->stagingBufferMemory, 0, particleIndicesBufferSize, 0, &memoryMappedData);
    printf("Memory mapped at address: %p\n", memoryMappedData);
    printf("Source vertex data address: %p\n", scene->modelsManager.pPackedModelVertex);
    printf("Copying %llu bytes from vertex data to staging buffer\n", (unsigned long long) particleIndicesBufferSize);

    for (int i = 0; i < 6; i++)
    {
        printf("Indice (%d) : %d", i, scene->particleManager.pPackedParticleIndices[i]);
    }
    memcpy(memoryMappedData, scene->particleManager.pPackedParticleIndices, (size_t) particleIndicesBufferSize);
    printf("Vertex data copied successfully to staging buffer\n");
    // assert(objects->sceneVertexLen == 24);

    vkUnmapMemory(ld->device, *dbm->stagingBufferMemory);
    printf("Staging buffer memory unmapped\n");

    dbm->indexBuffer       = malloc(sizeof(*dbm->indexBuffer));
    dbm->indexBufferMemory = malloc(sizeof(*dbm->indexBufferMemory));
    printf("Allocated device buffer pointers: buffer=%p, memory=%p\n", dbm->indexBuffer, dbm->indexBufferMemory);
    printf("Creating device-local geometry buffer with size %llu bytes\n",
           (unsigned long long) particleIndicesBufferSize);

    createVkBuffer(pd, ld->device, particleIndicesBufferSize,
                   VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_SHARING_MODE_EXCLUSIVE,
                   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, dbm->indexBuffer, dbm->indexBufferMemory);
    printf("Device buffer created: handle=%p, memory=%p\n", (void*) (uintptr_t) *dbm->indexBuffer,
           (void*) (uintptr_t) *dbm->indexBufferMemory);

    printf("Copying data from staging buffer to device buffer (%llu bytes)\n",
           (unsigned long long) particleIndicesBufferSize);
    copyBuffer(ld, *dbm->stagingBuffer, *dbm->indexBuffer, particleIndicesBufferSize, commandPool->transferCommandPool,
               *(ld->queueManager.transferQueue));
    printf("Buffer copy operation completed\n");

    printf("Cleaning up staging buffer resources\n");
    vkDestroyBuffer(ld->device, *dbm->stagingBuffer, VK_NULL_HANDLE);
    vkFreeMemory(ld->device, *dbm->stagingBufferMemory, VK_NULL_HANDLE);
    printf("Staging buffer destroyed and memory freed\n");
    printf("=== GEOMETRY BUFFER CREATION COMPLETE ===\n");
}
void createGeometryBuffer(PhysicalDevice* pd, LogicalDevice* ld, DeviceBufferManager* dbm, SCENE* scene,
                          CommandPools* commandPool)
{
    printf("=== CREATING GEOMETRY BUFFER ===\n");
    printf("PACKEDMODELVERTEXLEN %d\n", scene->modelsManager.packedModelVertexLen);
    printf("PACKEDMODELINDICESLEN %d\n", scene->modelsManager.packedModelIndex.itemsLen);

    VkDeviceSize geometryBufferSize =
        sizeof(*(scene->modelsManager.pPackedModelVertex)) * scene->modelsManager.packedModelVertexLen;
    printf("Geometry buffer size calculation: sizeof(vertex)=%zu * vertexCount=%d = %llu bytes (%.2f KB)\n",
           sizeof(*(scene->modelsManager.pPackedModelVertex)), scene->modelsManager.packedModelVertexLen,
           (unsigned long long) geometryBufferSize, (double) geometryBufferSize / 1024.0);

    dbm->stagingBuffer       = malloc(sizeof(*dbm->stagingBuffer));
    dbm->stagingBufferMemory = malloc(sizeof(*dbm->stagingBufferMemory));
    printf("Allocated staging buffer pointers: buffer=%p, memory=%p\n", dbm->stagingBuffer, dbm->stagingBufferMemory);
    printf("Creating staging buffer with size %llu bytes\n", (unsigned long long) geometryBufferSize);

    createVkBuffer(pd, ld->device, geometryBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_SHARING_MODE_EXCLUSIVE,
                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, dbm->stagingBuffer,
                   dbm->stagingBufferMemory);
    printf("Staging buffer created: handle=%p, memory=%p\n", (void*) (uintptr_t) *dbm->stagingBuffer,
           (void*) (uintptr_t) *dbm->stagingBufferMemory);

    void* memoryMappedData; // Dst memory to copy from buffer data to gpu memory
    printf("Mapping staging buffer memory for vertex data copy\n");
    vkMapMemory(ld->device, *dbm->stagingBufferMemory, 0, geometryBufferSize, 0, &memoryMappedData);
    printf("Memory mapped at address: %p\n", memoryMappedData);
    printf("Source vertex data address: %p\n", scene->modelsManager.pPackedModelVertex);
    printf("Copying %llu bytes from vertex data to staging buffer\n", (unsigned long long) geometryBufferSize);

    memcpy(memoryMappedData, scene->modelsManager.pPackedModelVertex, (size_t) geometryBufferSize);
    printf("Vertex data copied successfully to staging buffer\n");
    // assert(objects->sceneVertexLen == 24);

    vkUnmapMemory(ld->device, *dbm->stagingBufferMemory);
    printf("Staging buffer memory unmapped\n");

    dbm->buffer       = malloc(sizeof(*dbm->buffer));
    dbm->bufferMemory = malloc(sizeof(*dbm->bufferMemory));
    printf("Allocated device buffer pointers: buffer=%p, memory=%p\n", dbm->buffer, dbm->bufferMemory);
    printf("Creating device-local geometry buffer with size %llu bytes\n", (unsigned long long) geometryBufferSize);

    createVkBuffer(pd, ld->device, geometryBufferSize,
                   VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE,
                   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, dbm->buffer, dbm->bufferMemory);
    printf("Device buffer created: handle=%p, memory=%p\n", (void*) (uintptr_t) *dbm->buffer,
           (void*) (uintptr_t) *dbm->bufferMemory);

    printf("Copying data from staging buffer to device buffer (%llu bytes)\n", (unsigned long long) geometryBufferSize);
    copyBuffer(ld, *dbm->stagingBuffer, *dbm->buffer, geometryBufferSize, commandPool->transferCommandPool,
               *(ld->queueManager.transferQueue));
    printf("Buffer copy operation completed\n");

    printf("Cleaning up staging buffer resources\n");
    vkDestroyBuffer(ld->device, *dbm->stagingBuffer, VK_NULL_HANDLE);
    vkFreeMemory(ld->device, *dbm->stagingBufferMemory, VK_NULL_HANDLE);
    printf("Staging buffer destroyed and memory freed\n");
    printf("=== GEOMETRY BUFFER CREATION COMPLETE ===\n");
}

void createLightVolumeVertexBuffer(PhysicalDevice* pd, LogicalDevice* ld, DeviceBufferManager* dbm, SCENE* scene,
                                   CommandPools* commandPool)
{
    printf("=== CREATING LIGHT VOLUME VERTEX BUFFER ===\n");
    printf("Packed light count: %u\n", scene->lightManager.packedLightLen);

    VkDeviceSize lightsBufferSize = sizeof(*(scene->lightManager.pPackedLight)) * scene->lightManager.packedLightLen;
    printf("Light buffer size calculation: sizeof(light)=%zu * lightCount=%u = %llu bytes (%.2f KB)\n",
           sizeof(*(scene->lightManager.pPackedLight)), scene->lightManager.packedLightLen,
           (unsigned long long) lightsBufferSize, (double) lightsBufferSize / 1024.0);

    dbm->stagingBuffer       = malloc(sizeof(*dbm->stagingBuffer));
    dbm->stagingBufferMemory = malloc(sizeof(*dbm->stagingBufferMemory));
    printf("Allocated staging buffer pointers: buffer=%p, memory=%p\n", dbm->stagingBuffer, dbm->stagingBufferMemory);
    printf("Creating staging buffer for lights with size %llu bytes\n", (unsigned long long) lightsBufferSize);

    createVkBuffer(pd, ld->device, lightsBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_SHARING_MODE_EXCLUSIVE,
                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, dbm->stagingBuffer,
                   dbm->stagingBufferMemory);
    printf("Staging buffer created: handle=%p, memory=%p\n", (void*) (uintptr_t) *dbm->stagingBuffer,
           (void*) (uintptr_t) *dbm->stagingBufferMemory);

    void* memoryMappedData; // Dst memory to copy from buffer data to gpu memory
    printf("Mapping staging buffer memory for light data copy\n");
    vkMapMemory(ld->device, *dbm->stagingBufferMemory, 0, lightsBufferSize, 0, &memoryMappedData);
    printf("Memory mapped at address: %p\n", memoryMappedData);
    printf("Source light data address: %p\n", scene->lightManager.pPackedLight);
    printf("Copying %llu bytes from light data to staging buffer\n", (unsigned long long) lightsBufferSize);

    memcpy(memoryMappedData, scene->lightManager.pPackedLight, (size_t) lightsBufferSize);
    printf("Light data copied successfully to staging buffer\n");
    // assert(objects->sceneVertexLen == 24);

    vkUnmapMemory(ld->device, *dbm->stagingBufferMemory);
    printf("Staging buffer memory unmapped\n");

    dbm->buffer       = malloc(sizeof(*dbm->buffer));
    dbm->bufferMemory = malloc(sizeof(*dbm->buffer));
    printf("Allocated device buffer pointers: buffer=%p, memory=%p\n", dbm->buffer, dbm->bufferMemory);
    printf("Creating device-local light buffer with size %llu bytes\n", (unsigned long long) lightsBufferSize);

    createVkBuffer(pd, ld->device, lightsBufferSize,
                   VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE,
                   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, dbm->buffer, dbm->bufferMemory);
    printf("Device buffer created: handle=%p, memory=%p\n", (void*) (uintptr_t) *dbm->buffer,
           (void*) (uintptr_t) *dbm->bufferMemory);

    printf("Copying data from staging buffer to device buffer (%llu bytes)\n", (unsigned long long) lightsBufferSize);
    copyBuffer(ld, *dbm->stagingBuffer, *dbm->buffer, lightsBufferSize, commandPool->transferCommandPool,
               *(ld->queueManager.transferQueue));
    printf("Buffer copy operation completed\n");

    printf("Cleaning up staging buffer resources\n");
    vkDestroyBuffer(ld->device, *dbm->stagingBuffer, VK_NULL_HANDLE);
    vkFreeMemory(ld->device, *dbm->stagingBufferMemory, VK_NULL_HANDLE);
    printf("Staging buffer destroyed and memory freed\n");
    printf("=== LIGHT VOLUME VERTEX BUFFER CREATION COMPLETE ===\n");
}

void createLightVolumeIndiceBuffers(PhysicalDevice* pd, LogicalDevice* ld, DeviceBufferManager* dbm, SCENE* scene,
                                    CommandPools* commandPool)
{
    printf("=== CREATING LIGHT VOLUME INDEX BUFFER ===\n");
    printf("Packed light count: %u\n", scene->lightManager.packedLightLen);

    VkDeviceSize bufferSize =
        sizeof(*(scene->lightManager.packedLightIndex.items)) * scene->lightManager.packedLightIndex.itemsLen;
    printf("Light index buffer size calculation: sizeof(uint32_t)=%zu * lightCount=%u = %u bytes (%.2f KB)\n",
           sizeof(uint32_t), scene->lightManager.packedLightLen, bufferSize, (double) bufferSize / 1024.0);

    dbm->stagingBuffer       = malloc(sizeof(*dbm->stagingBuffer));
    dbm->stagingBufferMemory = malloc(sizeof(*dbm->stagingBufferMemory));
    printf("Allocated staging buffer pointers: buffer=%p, memory=%p\n", dbm->stagingBuffer, dbm->stagingBufferMemory);
    printf("Creating staging buffer for light indices with size %u bytes\n", bufferSize);

    createVkBuffer(pd, ld->device, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_SHARING_MODE_EXCLUSIVE,
                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, dbm->stagingBuffer,
                   dbm->stagingBufferMemory);
    printf("Staging buffer created: handle=%p, memory=%p\n", (void*) (uintptr_t) *dbm->stagingBuffer,
           (void*) (uintptr_t) *dbm->stagingBufferMemory);

    void* data;
    printf("Mapping staging buffer memory for light index data copy\n");
    vkMapMemory(ld->device, *dbm->stagingBufferMemory, 0, bufferSize, 0, &data);
    printf("Memory mapped at address: %p\n", data);
    printf("Source light data address: %p\n", scene->lightManager.pPackedLight);
    printf("Copying %u bytes from light data index to index staging buffer\n", bufferSize);

    memcpy(data, scene->lightManager.packedLightIndex.items, (size_t) bufferSize);
    printf("Light index data copied to staging buffer\n");

    vkUnmapMemory(ld->device, *dbm->stagingBufferMemory);
    printf("Staging buffer memory unmapped\n");

    dbm->indexBuffer       = malloc(sizeof(*dbm->indexBuffer));
    dbm->indexBufferMemory = malloc(sizeof(*dbm->indexBufferMemory));
    printf("Allocated index buffer pointers: buffer=%p, memory=%p\n", dbm->indexBuffer, dbm->indexBufferMemory);
    printf("Creating device-local light index buffer with size %u bytes\n", bufferSize);

    createVkBuffer(pd, ld->device, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                   VK_SHARING_MODE_EXCLUSIVE, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, dbm->indexBuffer,
                   dbm->indexBufferMemory);
    printf("Index buffer created: handle=%p, memory=%p\n", (void*) (uintptr_t) *dbm->indexBuffer,
           (void*) (uintptr_t) *dbm->indexBufferMemory);

    printf("Copying data from staging buffer to index buffer (%u bytes)\n", bufferSize);
    copyBuffer(ld, *dbm->stagingBuffer, *dbm->indexBuffer, bufferSize, commandPool->transferCommandPool,
               *(ld->queueManager.transferQueue));
    printf("Index buffer copy operation completed\n");

    printf("Cleaning up staging buffer resources\n");
    vkDestroyBuffer(ld->device, *dbm->stagingBuffer, VK_NULL_HANDLE);
    vkFreeMemory(ld->device, *dbm->stagingBufferMemory, VK_NULL_HANDLE);
    printf("Staging buffer destroyed and memory freed\n");
    printf("=== LIGHT VOLUME INDEX BUFFER CREATION COMPLETE ===\n");
}

void createParticleBuffer(PhysicalDevice* pd, LogicalDevice* ld, DeviceBufferManager* dbm, SCENE* scene,
                          CommandPools* commandPool)
{
    printf("=== CREATING PARTICLE BUFFER ===\n");

    VkDeviceSize particleBufferSize =
        sizeof(*(scene->particleManager.pPackedParticleVertex)) * scene->particleManager.packedParticleVertexLen;
    printf("Particle buffer size calculation: sizeof(vertex)=%zu * vertexCount=%d = %llu bytes (%.2f KB)\n",
           sizeof(*(scene->particleManager.pPackedParticleVertex)), scene->particleManager.packedParticleVertexLen,
           (unsigned long long) particleBufferSize, (double) particleBufferSize / 1024.0);

    dbm->stagingBuffer       = malloc(sizeof(*dbm->stagingBuffer));
    dbm->stagingBufferMemory = malloc(sizeof(*dbm->stagingBufferMemory));
    printf("Allocated staging buffer pointers: buffer=%p, memory=%p\n", dbm->stagingBuffer, dbm->stagingBufferMemory);
    printf("Creating staging buffer with size %llu bytes\n", (unsigned long long) particleBufferSize);

    createVkBuffer(pd, ld->device, particleBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_SHARING_MODE_EXCLUSIVE,
                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, dbm->stagingBuffer,
                   dbm->stagingBufferMemory);
    printf("Staging buffer created: handle=%p, memory=%p\n", (void*) (uintptr_t) *dbm->stagingBuffer,
           (void*) (uintptr_t) *dbm->stagingBufferMemory);

    void* memoryMappedData; // Dst memory to copy from buffer data to gpu memory
    printf("Mapping staging buffer memory for vertex data copy\n");
    vkMapMemory(ld->device, *dbm->stagingBufferMemory, 0, particleBufferSize, 0, &memoryMappedData);
    printf("Memory mapped at address: %p\n", memoryMappedData);
    printf("Source vertex data address: %p\n", scene->particleManager.pPackedParticleVertex);
    printf("Copying %llu bytes from vertex data to staging buffer\n", (unsigned long long) particleBufferSize);

    printTrzVertex(scene->particleManager.pPackedParticleVertex, particleBufferSize / 64);
    memcpy(memoryMappedData, scene->particleManager.pPackedParticleVertex, (size_t) particleBufferSize);
    printf("Vertex data copied successfully to staging buffer\n");
    // assert(objects->sceneVertexLen == 24);

    vkUnmapMemory(ld->device, *dbm->stagingBufferMemory);
    printf("Staging buffer memory unmapped\n");

    dbm->buffer       = malloc(sizeof(*dbm->buffer));
    dbm->bufferMemory = malloc(sizeof(*dbm->bufferMemory));
    printf("Allocated device buffer pointers: buffer=%p, memory=%p\n", dbm->buffer, dbm->bufferMemory);
    printf("Creating device-local geometry buffer with size %llu bytes\n", (unsigned long long) particleBufferSize);

    createVkBuffer(pd, ld->device, particleBufferSize,
                   VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE,
                   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, dbm->buffer, dbm->bufferMemory);
    printf("Device buffer created: handle=%p, memory=%p\n", (void*) (uintptr_t) *dbm->buffer,
           (void*) (uintptr_t) *dbm->bufferMemory);

    printf("Copying data from staging buffer to device buffer (%llu bytes)\n", (unsigned long long) particleBufferSize);
    copyBuffer(ld, *dbm->stagingBuffer, *dbm->buffer, particleBufferSize, commandPool->transferCommandPool,
               *(ld->queueManager.transferQueue));
    printf("Buffer copy operation completed\n");

    printf("Cleaning up staging buffer resources\n");
    vkDestroyBuffer(ld->device, *dbm->stagingBuffer, VK_NULL_HANDLE);
    vkFreeMemory(ld->device, *dbm->stagingBufferMemory, VK_NULL_HANDLE);
    printf("Staging buffer destroyed and memory freed\n");
    printf("=== Particle BUFFER CREATION COMPLETE ===\n");
}
