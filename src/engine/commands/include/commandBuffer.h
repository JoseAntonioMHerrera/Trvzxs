#pragma once
#include <vulkan/vulkan.h>
#include <stddef.h>
#include <stdio.h>
#include "transformations.h"
#include "image.h"
#include "vertex.h"
#include "compilerMacros.h"
#include "numberTypes.h"
#include "miscTypes.h"
#include "deviceManagment.h"
#include "geometryPipelineTypes.h"
#include <string.h> //TODO revisar memcpy de aqui

VkCommandBuffer beginSingleTimeCommands(LogicalDevice *ld, VkCommandPool commandPool);
/*
VkCommandBuffer beginSingleTimeCommands(LogicalDevice *ld, VkCommandPool commandPool) {
    printf("BEGIN SINGLETIME COMMAND FLAG 1\n");
    VkCommandBufferAllocateInfo allocInfo =
    {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandPool = commandPool,
        .commandBufferCount = 1,
    };


    VkCommandBuffer commandBuffer;
    printf("BEGIN SINGLETIME COMMAND FLAG 2\n");
    vkAllocateCommandBuffers(ld->device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo =
    {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
printf("BEGIN SINGLETIME COMMAND FLAG 3\n");
    vkBeginCommandBuffer(commandBuffer, &beginInfo);
printf("BEGIN SINGLETIME COMMAND FLAG 4\n");
    return commandBuffer;
}*/
void endSingleTimeCommands(VkCommandBuffer commandBuffer, VkCommandPool commandPool, VkQueue queue, VkDevice device);
/*
void endSingleTimeCommands(VkCommandBuffer commandBuffer, VkCommandPool commandPool, VkQueue graphicsQueue, VkDevice device) {
    printf("end SINGLETIME COMMAND FLAG 1\n");
    vkEndCommandBuffer(commandBuffer);
    printf("end SINGLETIME COMMAND FLAG 1\n");
    VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &commandBuffer,
    };

    printf("end SINGLETIME COMMAND FLAG 1\n");
    vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        printf("end SINGLETIME COMMAND FLAG 1\n");
    vkQueueWaitIdle(graphicsQueue);
    printf("end SINGLETIME COMMAND FLAG 1\n");
    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
        printf("end SINGLETIME COMMAND FLAG 1\n");
}*/
// queue: queue where to submit
void copyBufferToImage(LogicalDevice *ld, VkDevice device, VkCommandPool commandPool, VkQueue queue, VkBuffer buffer,
                       VkImage image, uint32_t width, uint32_t height);
/*
void copyBufferToImage(LogicalDevice *ld, VkDevice device, VkCommandPool commandPool, VkQueue queue, VkBuffer buffer,
                       VkImage image, uint32_t width, uint32_t height) {

    VkCommandBuffer commandBuffer = beginSingleTimeCommands(ld, commandPool);

    VkBufferImageCopy region = {

    };
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;

    region.imageOffset.x = 0;
    region.imageOffset.y = 0;
    region.imageOffset.z = 0;

    region.imageExtent.width = width;
    region.imageExtent.height = height;
    region.imageExtent.depth = 1;

    vkCmdCopyBufferToImage(
    commandBuffer,
    buffer,
    image,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    1,
    &region
    );
    // queue: the queue to process the command operations
    endSingleTimeCommands(commandBuffer,commandPool, queue, device);
    printf("COPYBUFFER TO IMAGE FLAG 1\n");

}*/



void copyBuffer(LogicalDevice *ld, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, VkCommandPool commandPool, VkQueue queue);

// TRANSFER OPERATIONS TODO MAKE IT USEFUL
/*
TRVZXS_inline void copyBuffer(LogicalDevice *ld, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, VkCommandPool *transferCommandPool) {
        printf("COPYBUFFER FLAG 1\n");

    /*VkCommandBufferAllocateInfo allocInfo;
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

    VkCommandBuffer transferCommandBuffer = beginSingleTimeCommands(ld, transferCommandPool);
            printf("COPYBUFFER FLAG 1\n");
    vkCmdCopyBuffer(transferCommandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
            printf("COPYBUFFER FLAG 1\n");
    endSingleTimeCommands(transferCommandBuffer, transferCommandPool, *ld->queueManager.graphicsQueue, ld->device);
        printf("COPYBUFFER FLAG 1\n");
    printf("COPYBUFFER FLAG 4\n");


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
}*/
void createCommandBuffer(GeometryDisplayPipelineManager *gdp, VkDevice *device, size_t framesInFlight);

/*TRVZXS_inline void createCommandBuffer(GeometryDisplayPipelineManager *gdp, VkDevice *device, size_t framesInFlight){
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
}*/
