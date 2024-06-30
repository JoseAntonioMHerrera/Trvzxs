#ifndef _TRVZXS_BUFFER_H_
#define _TRVZXS_BUFFER_H_
#include <stdio.h>
#include "vulkan.h"
#include "transformations.h"
#include "compilerMacros.h"
#include "numberTypes.h"
#include "miscTypes.h"
#include <stddef.h>
#include "vulkanHut.h"
#include <deviceManagment.h>

TRVZXS_inline void createVkBuffer(PhysicalDeviceMemoryManager *pdmm, VkDeviceSize size, VkBufferUsageFlags usage, VkSharingMode sharingMode, VkMemoryPropertyFlags properties,
                    VkBuffer *buffer, VkDeviceMemory *bufferMemory) {

    VkBufferCreateInfo bufferInfo;
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = sharingMode;
    bufferInfo.flags = 0;
    bufferInfo.pNext = TRVZXS_NULL;

    if (vkCreateBuffer(pdmm->devices[pdmm->deviceDefaultIndex], &bufferInfo, VK_NULL_HANDLE, buffer) != VK_SUCCESS) {
        printf("Failed to create VkBuffer instance!");
        exit(1);
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(pdmm->devices[pdmm->deviceDefaultIndex], *buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo;
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(pdmm, memRequirements.memoryTypeBits, properties);
    allocInfo.pNext = VK_NULL_HANDLE;

    if (vkAllocateMemory(pdmm->devices[pdmm->deviceDefaultIndex], &allocInfo, VK_NULL_HANDLE, bufferMemory) != VK_SUCCESS) {
        printf("Failed to allocate memory for VkBuffer");
        exit(1);
    }

    vkBindBufferMemory(pdmm->devices[pdmm->deviceDefaultIndex], *buffer, *bufferMemory, 0);
}

#endif
