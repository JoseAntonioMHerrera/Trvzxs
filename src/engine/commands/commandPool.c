#include "commandPool.h"
#include "computePipeline.h"
void createGraphicsCommandPool(PhysicalDevice* pd, VkDevice device, GraphicPipelineToSurface *gdp) {

    VkCommandPoolCreateInfo poolInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = pd->queueFamilyIndices.graphicsFamily,
    };

    //gdp->commandPool->graphicsCommandPool = malloc(sizeof(*gdp->commandPool->graphicsCommandPool));

    if (vkCreateCommandPool(device, &poolInfo, VK_NULL_HANDLE, &(gdp->commandPool->graphicsCommandPool)) != VK_SUCCESS) {

        exit(0);
    }
}


void createTransferQueueCommandPool(PhysicalDevice* pd, VkDevice device, GraphicPipelineToSurface *gdp) {
    VkCommandPoolCreateInfo transferPoolInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
        .queueFamilyIndex = pd->queueFamilyIndices.transferFamily,
    };

    //gdp->commandPool->transferCommandPool = malloc(sizeof(*gdp->commandPool->transferCommandPool));
    if (vkCreateCommandPool(device, &transferPoolInfo, VK_NULL_HANDLE, &(gdp->commandPool->transferCommandPool)) != VK_SUCCESS) {

        exit(0);
    }
}
void createComputeQueueCommandPool(PhysicalDevice* pd, VkDevice device, ComputePipeline* computeP){
        VkCommandPoolCreateInfo computePoolInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
        .queueFamilyIndex = pd->queueFamilyIndices.computeFamily,
    };

//    compP->commandPool->transferCommandPool = malloc(sizeof(*compP->commandPool->transferCommandPool));
    if (vkCreateCommandPool(device, &computePoolInfo, VK_NULL_HANDLE, &(computeP->commandPool->computeCommandPool)) != VK_SUCCESS) {

        exit(0);
    }
}
