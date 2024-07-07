#pragma once

#include <vulkan/vulkan.h>
#include <stddef.h>
#include <stdio.h>
#include "../../../include/geometry/transformations.h"
#include "../../../include/geometry/vertex.h"
#include "../../../include/langDefinitions/compilerMacros.h"
#include "../../../include/langDefinitions/numberTypes.h"
#include "../../../include/langDefinitions/miscTypes.h"
#include "../../device/include/deviceManagment.h"
#include "../../commands/include/commandBuffer.h"
#include <string.h> //TODO revisar memcpy de aqui

TRVZXS_inline void createVkBuffer(PhysicalDevice* pd, VkDevice d, VkDeviceSize size, VkBufferUsageFlags usage, VkSharingMode sharingMode, VkMemoryPropertyFlags properties,
                    VkBuffer* buffer, VkDeviceMemory* bufferMemory) {

    VkBufferCreateInfo bufferInfo;
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = sharingMode;
    bufferInfo.flags = 0;
    bufferInfo.pNext = TRVZXS_NULL;

    if (vkCreateBuffer(d, &bufferInfo, VK_NULL_HANDLE, buffer) != VK_SUCCESS) {
        printf("Failed to create VkBuffer instance!");
        exit(1);
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(d, *buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo;
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(pd, memRequirements.memoryTypeBits, properties);
    allocInfo.pNext = VK_NULL_HANDLE;

    if (vkAllocateMemory(d, &allocInfo, VK_NULL_HANDLE, bufferMemory) != VK_SUCCESS) {
        printf("Failed to allocate memory for VkBuffer");
        exit(1);
    }

    vkBindBufferMemory(d, *buffer, *bufferMemory, 0);
}

TRVZXS_inline void createIndiceBuffer(PhysicalDevice* pd, LogicalDevice* ld, DeviceBufferManager *dbm, GEOM_OBJECT *objects, VkBuffer *stagingIndicesBuffer,
VkDeviceMemory* stagingIndicesBufferMemory) {
    uint32_t bufferSize = sizeof(uint32_t) * objects->indicesLen;


    createVkBuffer(pd, ld->device, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_SHARING_MODE_EXCLUSIVE,
    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingIndicesBuffer,
    stagingIndicesBufferMemory);

    void* data;
    vkMapMemory(ld->device, *stagingIndicesBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, objects->indices, (size_t) bufferSize);
    vkUnmapMemory(ld->device, *stagingIndicesBufferMemory);

    createVkBuffer(pd, ld->device, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE,
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, dbm->indexBuffer, dbm->indexBufferMemory);

    copyBuffer(ld, *stagingIndicesBuffer, *dbm->indexBuffer, bufferSize, ld->displayManager->commandPools->transferCommandPool);

    vkDestroyBuffer(ld->device, *stagingIndicesBuffer, VK_NULL_HANDLE);
    vkFreeMemory(ld->device, *stagingIndicesBufferMemory, VK_NULL_HANDLE);

}

TRVZXS_inline void createGeometryBuffer(PhysicalDevice *pd, LogicalDevice *ld, DeviceBufferManager *dbm, GEOM_OBJECT *objects, VkBuffer *stagingBuffer,
VkDeviceMemory *stagingBufferMemory){

    uint32_t geometryBufferSize = sizeof(*(objects->data))*objects->dataLen;

    createVkBuffer(pd, ld->device, geometryBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_SHARING_MODE_EXCLUSIVE,
    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void* memoryMappedData; //Dst memory to copy from buffer data to gpu memory
    vkMapMemory(ld->device, *stagingBufferMemory, 0, geometryBufferSize, 0, &memoryMappedData);
    memcpy(memoryMappedData, objects->data, (size_t) geometryBufferSize);
    vkUnmapMemory(ld->device, *stagingBufferMemory);
    createVkBuffer(pd, ld->device, geometryBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE,
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, dbm->geometryBuffer, dbm->geometryBufferMemory);

    copyBuffer(ld, *dbm->stagingBuffer, *dbm->geometryBuffer, geometryBufferSize, ld->displayManager->commandPools->transferCommandPool);

    vkDestroyBuffer(ld->device, *dbm->stagingBuffer, VK_NULL_HANDLE);
    vkFreeMemory(ld->device, *dbm->stagingBufferMemory, VK_NULL_HANDLE);


}
