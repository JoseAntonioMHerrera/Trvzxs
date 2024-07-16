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
        printf("COPYBUFFER FLAG 1\n");

    VkCommandBufferAllocateInfo allocInfo;
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = *transferCommandPool;
    allocInfo.commandBufferCount = 1;
    allocInfo.pNext = VK_NULL_HANDLE;
    printf("COPYBUFFER FLAG 1\n");
    VkCommandBuffer transferCommandBuffer;
    vkAllocateCommandBuffers(ld->device, &allocInfo, &transferCommandBuffer);
        printf("COPYBUFFER FLAG 2\n");

    VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, // Command only executed once
    };


    vkBeginCommandBuffer(transferCommandBuffer, &beginInfo);
    printf("COPYBUFFER FLAG 3\n");

    VkBufferCopy copyRegion = {
        .srcOffset = 0, // Optional
        .dstOffset = 0, // Optional
        .size = size,
    };

    printf("COPYBUFFER FLAG 4\n");

    vkCmdCopyBuffer(transferCommandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
    vkEndCommandBuffer(transferCommandBuffer);
    VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &transferCommandBuffer,
        .pNext = VK_NULL_HANDLE,
    };


    vkQueueSubmit(*ld->queueManager.transferQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(*ld->queueManager.transferQueue);
    printf("COPYBUFFER FLAG 4\n");
    vkFreeCommandBuffers(ld->device, *transferCommandPool, 1, &transferCommandBuffer);
}

TRVZXS_inline void createCommandBuffer(GeometryDisplayPipelineManager *gdp, VkDevice *device, size_t framesInFlight){
    gdp->commandLen = framesInFlight;
    gdp->commandBuffer = malloc(sizeof(VkCommandBuffer)* gdp->commandLen);
    VkCommandBufferAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = *gdp->commandPool->graphicsCommandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = gdp->maxFrames,
        .pNext = VK_NULL_HANDLE,
    };
    printf("HOLA ESTAMOS EN CREATECOMMANDBUFFER\n");
    if (vkAllocateCommandBuffers(*device, &allocInfo, gdp->commandBuffer) != VK_SUCCESS) {
        printf("Error al crear el command buffer");
        exit(0);
    }
}
