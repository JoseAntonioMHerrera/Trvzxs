#include "buffer.h"


// TODO when the time comes, update the behaviour of staging buffers of buffer creation functions

void createVkBuffer(PhysicalDevice* pd, VkDevice d, VkDeviceSize size, VkBufferUsageFlags usage, VkSharingMode sharingMode, VkMemoryPropertyFlags properties,
                    VkBuffer* buffer, VkDeviceMemory* bufferMemory) {

    VkBufferCreateInfo bufferInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = usage,
        .sharingMode = sharingMode,
        .flags = 0,
        .pNext = VK_NULL_HANDLE,
    };


    if (vkCreateBuffer(d, &bufferInfo, VK_NULL_HANDLE, buffer) != VK_SUCCESS) {

        exit(1);
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(d, *buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memRequirements.size,
        .memoryTypeIndex = findMemoryType(pd, memRequirements.memoryTypeBits, properties),
        .pNext = VK_NULL_HANDLE,

    };


    if (vkAllocateMemory(d, &allocInfo, VK_NULL_HANDLE, bufferMemory) != VK_SUCCESS) {

        exit(1);
    }

    vkBindBufferMemory(d, *buffer, *bufferMemory, 0);

}

void createGeometryIndiceBuffer(PhysicalDevice* pd, LogicalDevice* ld, DeviceBufferManager *dbm, SCENE *objects,
                                      CommandPools* commandPool) {

    VkDeviceSize bufferSize = sizeof(uint32_t) * objects->modelsManager.packedModelIndex.itemsLen;

    dbm->stagingBuffer = malloc(sizeof(*dbm->stagingBuffer));
    dbm->stagingBufferMemory = malloc(sizeof(*dbm->stagingBufferMemory));
    createVkBuffer(pd, ld->device, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_SHARING_MODE_EXCLUSIVE,
    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, dbm->stagingBuffer,
    dbm->stagingBufferMemory);

    void* data;
    vkMapMemory(ld->device, *dbm->stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, objects->modelsManager.pPackedModelVertex, (VkDeviceSize) bufferSize);
    vkUnmapMemory(ld->device, *dbm->stagingBufferMemory);
    dbm->indexBuffer = malloc(sizeof(*dbm->buffer));
    dbm->indexBufferMemory = malloc(sizeof(*dbm->buffer));
    createVkBuffer(pd, ld->device, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE,
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, dbm->indexBuffer, dbm->indexBufferMemory);

    copyBuffer(ld, *dbm->stagingBuffer, *dbm->indexBuffer, bufferSize, commandPool->transferCommandPool, *(ld->queueManager.transferQueue));

    vkDestroyBuffer(ld->device, *dbm->stagingBuffer, VK_NULL_HANDLE);
    vkFreeMemory(ld->device, *dbm->stagingBufferMemory, VK_NULL_HANDLE);

}

void createGeometryBuffer(PhysicalDevice *pd, LogicalDevice *ld, DeviceBufferManager *dbm, SCENE *scene, CommandPools* commandPool){
    printf("PACKEDMODELVERTEXLEN %d\n", scene->modelsManager.packedModelVertexLen);
        printf("PACKEDMODELINDICESLEN %d\n", scene->modelsManager.packedModelIndex.itemsLen);
    VkDeviceSize geometryBufferSize = sizeof(*(scene->modelsManager.pPackedModelVertex)) * scene->modelsManager.packedModelVertexLen;

    dbm->stagingBuffer = malloc(sizeof(*dbm->stagingBuffer));
    dbm->stagingBufferMemory = malloc(sizeof(*dbm->stagingBufferMemory));
    printf("FLAG 1\n", scene->modelsManager.packedModelVertexLen);
    createVkBuffer(pd, ld->device, geometryBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_SHARING_MODE_EXCLUSIVE,
    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, dbm->stagingBuffer, dbm->stagingBufferMemory);

    void* memoryMappedData; //Dst memory to copy from buffer data to gpu memory
    printf("FLAG 2\n", scene->modelsManager.packedModelVertexLen);
    vkMapMemory(ld->device, *dbm->stagingBufferMemory, 0, geometryBufferSize, 0, &memoryMappedData);

    memcpy(memoryMappedData, scene->modelsManager.pPackedModelVertex, (size_t) geometryBufferSize);
    // assert(objects->sceneVertexLen == 24);

    vkUnmapMemory(ld->device, *dbm->stagingBufferMemory);

    dbm->buffer = malloc(sizeof(*dbm->buffer));
    dbm->bufferMemory = malloc(sizeof(*dbm->buffer));

    createVkBuffer(pd, ld->device, geometryBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE,
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, dbm->buffer, dbm->bufferMemory);

    copyBuffer(ld, *dbm->stagingBuffer, *dbm->buffer, geometryBufferSize, commandPool->transferCommandPool, *(ld->queueManager.transferQueue));

    printf("FLAG 3\n", scene->modelsManager.packedModelVertexLen);
    vkDestroyBuffer(ld->device, *dbm->stagingBuffer, VK_NULL_HANDLE);
    vkFreeMemory(ld->device, *dbm->stagingBufferMemory, VK_NULL_HANDLE);
}

void createLightVolumeVertexBuffer(PhysicalDevice *pd, LogicalDevice *ld, DeviceBufferManager *dbm, SCENE *scene,
                                        CommandPools* commandPool){

    VkDeviceSize lightsBufferSize = sizeof(*(scene->lightManager.pPackedLight)) * scene->lightManager.packedLightLen;

    dbm->stagingBuffer = malloc(sizeof(*dbm->stagingBuffer));
    dbm->stagingBufferMemory = malloc(sizeof(*dbm->stagingBufferMemory));
    createVkBuffer(pd, ld->device, lightsBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_SHARING_MODE_EXCLUSIVE,
    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, dbm->stagingBuffer, dbm->stagingBufferMemory);

    void* memoryMappedData; //Dst memory to copy from buffer data to gpu memory

    vkMapMemory(ld->device, *dbm->stagingBufferMemory, 0, lightsBufferSize, 0, &memoryMappedData);

    memcpy(memoryMappedData, scene->lightManager.pPackedLight, (size_t) lightsBufferSize);
    // assert(objects->sceneVertexLen == 24);

    vkUnmapMemory(ld->device, *dbm->stagingBufferMemory);

    dbm->buffer = malloc(sizeof(*dbm->buffer));
    dbm->bufferMemory = malloc(sizeof(*dbm->buffer));

    createVkBuffer(pd, ld->device, lightsBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE,
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, dbm->buffer, dbm->bufferMemory);

    copyBuffer(ld, *dbm->stagingBuffer, *dbm->buffer, lightsBufferSize, commandPool->transferCommandPool, *(ld->queueManager.transferQueue));


    vkDestroyBuffer(ld->device, *dbm->stagingBuffer, VK_NULL_HANDLE);
    vkFreeMemory(ld->device, *dbm->stagingBufferMemory, VK_NULL_HANDLE);
}

void createLightVolumeIndiceBuffers(PhysicalDevice* pd, LogicalDevice* ld, DeviceBufferManager *dbm, SCENE *scene,
                                      CommandPools* commandPool) {

    uint32_t bufferSize = sizeof(uint32_t) * scene->lightManager.packedLightLen;

    dbm->stagingBuffer = malloc(sizeof(*dbm->stagingBuffer));
    dbm->stagingBufferMemory = malloc(sizeof(*dbm->stagingBufferMemory));
    createVkBuffer(pd, ld->device, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_SHARING_MODE_EXCLUSIVE,
    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, dbm->stagingBuffer,
    dbm->stagingBufferMemory);

    void* data;
    vkMapMemory(ld->device, *dbm->stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, scene->lightManager.pPackedLight, (size_t) bufferSize);
    vkUnmapMemory(ld->device, *dbm->stagingBufferMemory);
    dbm->indexBuffer = malloc(sizeof(*dbm->buffer));
    dbm->indexBufferMemory = malloc(sizeof(*dbm->buffer));
    createVkBuffer(pd, ld->device, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE,
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, dbm->indexBuffer, dbm->indexBufferMemory);

    copyBuffer(ld, *dbm->stagingBuffer, *dbm->indexBuffer, bufferSize, commandPool->transferCommandPool, *(ld->queueManager.transferQueue));

    vkDestroyBuffer(ld->device, *dbm->stagingBuffer, VK_NULL_HANDLE);
    vkFreeMemory(ld->device, *dbm->stagingBufferMemory, VK_NULL_HANDLE);

}
