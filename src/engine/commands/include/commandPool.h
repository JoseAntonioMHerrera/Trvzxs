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

TRVZXS_inline void createGraphicsCommandPool(PhysicalDevice* pd, VkDevice device, GeometryDisplayPipelineManager *gdp) {

    VkCommandPoolCreateInfo poolInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = pd->queueFamilyIndices.graphicsFamily,
    };

    //gdp->commandPool->graphicsCommandPool = malloc(sizeof(*gdp->commandPool->graphicsCommandPool));

    if (vkCreateCommandPool(device, &poolInfo, VK_NULL_HANDLE, &(gdp->commandPool->graphicsCommandPool)) != VK_SUCCESS) {
        printf("Error al crear el command pool");
        exit(0);
    }
}

TRVZXS_inline void createTransferQueueCommandPool(PhysicalDevice* pd, VkDevice device, GeometryDisplayPipelineManager *gdp) {
    VkCommandPoolCreateInfo transferPoolInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
        .queueFamilyIndex = pd->queueFamilyIndices.transferFamily,
    };

    //gdp->commandPool->transferCommandPool = malloc(sizeof(*gdp->commandPool->transferCommandPool));
    if (vkCreateCommandPool(device, &transferPoolInfo, VK_NULL_HANDLE, &(gdp->commandPool->transferCommandPool)) != VK_SUCCESS) {
        printf("Error al crear el command pool");
        exit(0);
    }
}
