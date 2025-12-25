#include "commandBuffer.h"

VkCommandBuffer beginSingleTimeCommands(LogicalDevice* ld, VkCommandPool commandPool)
{

    VkCommandBufferAllocateInfo allocInfo = {
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandPool        = commandPool,
        .commandBufferCount = 1,
    };

    VkCommandBuffer commandBuffer;

    vkAllocateCommandBuffers(ld->device, &allocInfo, &commandBuffer);
    VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

void endSingleTimeCommands(VkCommandBuffer commandBuffer, VkCommandPool commandPool, VkQueue queue, VkDevice device)
{

    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo = {
        .sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers    = &commandBuffer,
    };

    vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);

    vkQueueWaitIdle(queue);

    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}
// queue: queue where to submit
void copyBufferToImage(LogicalDevice* ld, VkDevice device, VkCommandPool commandPool, VkQueue queue, VkBuffer buffer,
                       VkImage image, uint32_t width, uint32_t height)
{

    VkCommandBuffer commandBuffer = beginSingleTimeCommands(ld, commandPool);

    VkBufferImageCopy region = {

    };
    region.bufferOffset      = 0;
    region.bufferRowLength   = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel       = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount     = 1;

    region.imageOffset.x = 0;
    region.imageOffset.y = 0;
    region.imageOffset.z = 0;

    region.imageExtent.width  = width;
    region.imageExtent.height = height;
    region.imageExtent.depth  = 1;

    vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    // queue: the queue to process the command operations
    endSingleTimeCommands(commandBuffer, commandPool, queue, device);
}

// TRANSFER OPERATIONS TODO MAKE IT USEFUL
void copyBuffer(LogicalDevice* ld, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, VkCommandPool commandPool,
                VkQueue queue)
{

    /*VkCommandBufferAllocateInfo allocInfo;
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = *transferCommandPool;
    allocInfo.commandBufferCount = 1;
    allocInfo.pNext = VK_NULL_HANDLE;

    VkCommandBuffer transferCommandBuffer;
    vkAllocateCommandBuffers(ld->device, &allocInfo, &transferCommandBuffer);


    VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, // Command only executed once
    };


    vkBeginCommandBuffer(transferCommandBuffer, &beginInfo);
*/

    VkBufferCopy copyRegion = {
        .srcOffset = 0, // Optional
        .dstOffset = 0, // Optional
        .size      = size,
    };
    VkCommandBuffer transferCommandBuffer = beginSingleTimeCommands(ld, commandPool);
    vkCmdCopyBuffer(transferCommandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    endSingleTimeCommands(transferCommandBuffer, commandPool, queue, ld->device);
    /*



        vkEndCommandBuffer(transferCommandBuffer);
        VkSubmitInfo submitInfo = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .commandBufferCount = 1,
            .pCommandBuffers = &transferCommandBuffer,
            .pNext = VK_NULL_HANDLE,
        };


        vkQueueSubmit(*ld->queueManager.transferQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(*ld->queueManager.transferQueue);

        vkFreeCommandBuffers(ld->device, *transferCommandPool, 1, &transferCommandBuffer);*/
}

void createCommandBuffer(GraphicPipelineToSurface* gdp, VkDevice* device, size_t framesInFlight)
{
    printf("HOLAHOLA\n");
    gdp->commandLen = framesInFlight;
    printf("HOLAHOLA\n");
    printf("HOLAHOLA\n");

    gdp->commandBuffer = malloc(sizeof(VkCommandBuffer) * gdp->commandLen);
    printf("HOLAHOLA\n");

    VkCommandBufferAllocateInfo allocGraphicCommandBufferInfo = {
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool        = gdp->commandPool->graphicsCommandPool,
        .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = gdp->maxFrames,
        .pNext              = VK_NULL_HANDLE,
    };
    printf("HOLAHOLA\n");

    if (vkAllocateCommandBuffers(*device, &allocGraphicCommandBufferInfo, gdp->commandBuffer) != VK_SUCCESS)
    {
        printf("HOLAHOLA\n");

        exit(0);
    }
    printf("HOLAHOLA\n");
}

void createCommandBufferWithCompute(GraphicPipelineToSurface* gdp, VkDevice* device, size_t framesInFlight)
{
    gdp->commandLen                                           = framesInFlight;
    gdp->computeP->commandBuffer                              = malloc(sizeof(VkCommandBuffer) * gdp->commandLen);
    gdp->commandBuffer                                        = malloc(sizeof(VkCommandBuffer) * gdp->commandLen);
    VkCommandBufferAllocateInfo allocGraphicCommandBufferInfo = {
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool        = gdp->commandPool->graphicsCommandPool,
        .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = gdp->maxFrames,
        .pNext              = VK_NULL_HANDLE,
    };

    if (vkAllocateCommandBuffers(*device, &allocGraphicCommandBufferInfo, gdp->commandBuffer) != VK_SUCCESS)
    {
        printf("ERROR creating commandBuffer for graphic!\n");
        exit(0);
    }

    for (uint32_t compIndex = 0; compIndex < gdp->computePipelineCount; compIndex++)
    {
        VkCommandBufferAllocateInfo allocComputeCommandBufferInfo = {
            .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool        = gdp->commandPool->computeCommandPool,
            .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = gdp->maxFrames,
            .pNext              = VK_NULL_HANDLE,
        };
        if (vkAllocateCommandBuffers(*device, &allocComputeCommandBufferInfo, gdp->computeP[compIndex].commandBuffer) !=
            VK_SUCCESS)
        {
            printf("ERROR creating commandBuffer for compute!\n");
            exit(1);
        }
    }
}
