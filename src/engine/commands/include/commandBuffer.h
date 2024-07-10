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
#include "geometryPipelineTypes.h"
#include <string.h> //TODO revisar memcpy de aqui

TRVZXS_inline void copyBuffer(LogicalDevice *ld, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, VkCommandPool *transferCommandPool) {
    VkCommandBufferAllocateInfo allocInfo;
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = *transferCommandPool;
    allocInfo.commandBufferCount = 1;
    allocInfo.pNext = VK_NULL_HANDLE;

    VkCommandBuffer transferCommandBuffer;
    vkAllocateCommandBuffers(ld->device, &allocInfo, &transferCommandBuffer);
    VkCommandBufferBeginInfo beginInfo;
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT; // Command only executed once

    vkBeginCommandBuffer(transferCommandBuffer, &beginInfo);

    VkBufferCopy copyRegion;
    copyRegion.srcOffset = 0; // Optional
    copyRegion.dstOffset = 0; // Optional
    copyRegion.size = size;

    vkCmdCopyBuffer(transferCommandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
    vkEndCommandBuffer(transferCommandBuffer);
    VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &transferCommandBuffer,
        .pNext = VK_NULL_HANDLE,
    };


    vkQueueSubmit(ld->queueManager.transferQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(ld->queueManager.transferQueue);

    vkFreeCommandBuffers(ld->device, *transferCommandPool, 1, &transferCommandBuffer);
}

TRVZXS_inline void createCommandBuffer(GeometryDisplayPipelineManager *ggp, VkDevice *device){
    ggp->commandBuffer = malloc(sizeof(VkCommandBuffer)* ggp->commandLen);
    VkCommandBufferAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = ggp->graphicsCommandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = ggp->maxFrames,
    };

    if (vkAllocateCommandBuffers(*device, &allocInfo, ggp->commandBuffer) != VK_SUCCESS) {
        printf("Error al crear el command buffer");
        exit(0);
    }
}
