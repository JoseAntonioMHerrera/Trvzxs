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
#include <string.h> //TODO revisar memcpy de aqui

TRVZXS_inline void createVkBuffer(PhysicalDevice* pd, VkDevice d, VkDeviceSize size, VkBufferUsageFlags usage, VkSharingMode sharingMode, VkMemoryPropertyFlags properties,
                    VkBuffer* buffer, VkDeviceMemory* bufferMemory) {
    printf("CREATEVKBUFFER FLAG 0\n");
    VkBufferCreateInfo bufferInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = usage,
        .sharingMode = sharingMode,
        .flags = 0,
        .pNext = VK_NULL_HANDLE,
    };

    printf("CREATEVKBUFFER FLAG 1\n");
    if (vkCreateBuffer(d, &bufferInfo, VK_NULL_HANDLE, buffer) != VK_SUCCESS) {
        printf("Failed to create VkBuffer instance!");
        exit(1);
    }
    printf("CREATEVKBUFFER FLAG 1\n");
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(d, *buffer, &memRequirements);
    printf("CREATEVKBUFFER FLAG 1\n");
    VkMemoryAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memRequirements.size,
        .memoryTypeIndex = findMemoryType(pd, memRequirements.memoryTypeBits, properties),
        .pNext = VK_NULL_HANDLE,

    };

    printf("CREATEVKBUFFER FLAG 1\n");
    if (vkAllocateMemory(d, &allocInfo, VK_NULL_HANDLE, bufferMemory) != VK_SUCCESS) {
        printf("Failed to allocate memory for VkBuffer");
        exit(1);
    }
    printf("CREATEVKBUFFER FLAG 1\n");
    vkBindBufferMemory(d, *buffer, *bufferMemory, 0);
    printf("CREATEVKBUFFER FLAG 10\n");
}

TRVZXS_inline void createIndiceBuffer(PhysicalDevice* pd, LogicalDevice* ld, DeviceBufferManager *dbm, GEOM_OBJECT *objects,
                                      CommandPools* commandPool) {
    uint32_t bufferSize = sizeof(uint32_t) * objects->indicesLen;

    dbm->stagingBuffer = malloc(sizeof(*dbm->stagingBuffer));
    dbm->stagingBufferMemory = malloc(sizeof(*dbm->stagingBufferMemory));
    createVkBuffer(pd, ld->device, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_SHARING_MODE_EXCLUSIVE,
    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, dbm->stagingBuffer,
    dbm->stagingBufferMemory);

    void* data;
    vkMapMemory(ld->device, *dbm->stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, objects->indices, (size_t) bufferSize);
    vkUnmapMemory(ld->device, *dbm->stagingBufferMemory);
    dbm->indexBuffer = malloc(sizeof(*dbm->buffer));
    dbm->indexBufferMemory = malloc(sizeof(*dbm->buffer));
    createVkBuffer(pd, ld->device, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE,
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, dbm->indexBuffer, dbm->indexBufferMemory);

    copyBuffer(ld, *dbm->stagingBuffer, *dbm->indexBuffer, bufferSize, commandPool->transferCommandPool, *(ld->queueManager.transferQueue));

    vkDestroyBuffer(ld->device, *dbm->stagingBuffer, VK_NULL_HANDLE);
    vkFreeMemory(ld->device, *dbm->stagingBufferMemory, VK_NULL_HANDLE);

}

TRVZXS_inline void createGeometryBuffer(PhysicalDevice *pd, LogicalDevice *ld, DeviceBufferManager *dbm, GEOM_OBJECT *objects,
                                        CommandPools* commandPool){

    VkDeviceSize geometryBufferSize = sizeof(*(objects->data))*objects->dataLen;

    dbm->stagingBuffer = malloc(sizeof(*dbm->stagingBuffer));
    dbm->stagingBufferMemory = malloc(sizeof(*dbm->stagingBufferMemory));
    createVkBuffer(pd, ld->device, geometryBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_SHARING_MODE_EXCLUSIVE,
    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, dbm->stagingBuffer, dbm->stagingBufferMemory);
    printf("CREATEGEOMETRYBYFFER FKAG 1\n");
    void* memoryMappedData; //Dst memory to copy from buffer data to gpu memory
        printf("CREATEGEOMETRYBYFFER FKAG 1\n");
    vkMapMemory(ld->device, *dbm->stagingBufferMemory, 0, geometryBufferSize, 0, &memoryMappedData);
        printf("CREATEGEOMETRYBYFFER FKAG 1\n");
    memcpy(memoryMappedData, objects->data, (size_t) geometryBufferSize);
        printf("CREATEGEOMETRYBYFFER FKAG 1\n");
    vkUnmapMemory(ld->device, *dbm->stagingBufferMemory);
    printf("CREATEGEOMETRYBYFFER FKAG 1\n");
    dbm->buffer = malloc(sizeof(*dbm->buffer));
    dbm->bufferMemory = malloc(sizeof(*dbm->buffer));
    printf("CREATEGEOMETRYBYFFER FKAG 1\n");
    createVkBuffer(pd, ld->device, geometryBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE,
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, dbm->buffer, dbm->bufferMemory);
    printf("CREATEGEOMETRYBYFFER FKAG 23\n");
    copyBuffer(ld, *dbm->stagingBuffer, *dbm->buffer, geometryBufferSize, commandPool->transferCommandPool, *(ld->queueManager.transferQueue));
    printf("CREATEGEOMETRYBYFFER FKAG 2\n");

    vkDestroyBuffer(ld->device, *dbm->stagingBuffer, VK_NULL_HANDLE);
    vkFreeMemory(ld->device, *dbm->stagingBufferMemory, VK_NULL_HANDLE);
    printf("CREATEGEOMETRYBYFFER FKAG 3\n");


}
