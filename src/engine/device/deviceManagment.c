#include "deviceManagment.h"
#include <stdlib.h>
#include <assert.h>
#include <SDL.h>
uint32_t checkMemoryTypeCache(DeviceManager* pdmm){

}

uint32_t createMemoryTypeCache(DeviceManager* pdmm){

}

void getMatchingQueueIndices(PhysicalDevice *pd, LogicalDevice* ld, VkQueueFamilyProperties* properties, int numProperties, QueueFamilyIndices *queuesIndices) {

    assert(queuesIndices);

    uint32_t currentLessOptimalGraphicQueueIndice = UINT32_MAX;
    uint32_t currentLessOptimalTransferQueueIndice = UINT32_MAX;
    uint32_t currentLessOptimalPresentSupportQueueIndice = UINT32_MAX;
    VkBool32 presentSupport = SDL_FALSE;

    for (int i = 0; i < numProperties; i++) {
        if ((properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT ) == VK_QUEUE_GRAPHICS_BIT
           && queuesIndices->graphicsFamily == UINT32_MAX) {

            VkResult result = vkGetPhysicalDeviceSurfaceSupportKHR(pd->gpu, i, ld->displayManager->surface, &presentSupport);
            assert(result == VK_SUCCESS);

            if(presentSupport){

                queuesIndices->graphicsFamily = i;
                queuesIndices->presentFamily = i;
                presentSupport = SDL_FALSE;
            }else{

                currentLessOptimalGraphicQueueIndice = i;
            }

        }else if((properties[i].queueFlags & VK_QUEUE_TRANSFER_BIT ) == VK_QUEUE_TRANSFER_BIT && queuesIndices->transferFamily == UINT32_MAX){

            currentLessOptimalTransferQueueIndice = i;

            if((properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT ) != VK_QUEUE_GRAPHICS_BIT &&
              (properties[i].queueFlags & VK_QUEUE_COMPUTE_BIT ) != VK_QUEUE_COMPUTE_BIT ) {

                queuesIndices->transferFamily =  i;
            }

        }else if(queuesIndices->presentFamily == UINT32_MAX){

            vkGetPhysicalDeviceSurfaceSupportKHR(pd->gpu, i, ld->displayManager->surface, &presentSupport);

            if(presentSupport){
                currentLessOptimalPresentSupportQueueIndice = i;
                presentSupport = VK_FALSE;

            }
        }else if(queuesIndices->transferFamily != UINT32_MAX && queuesIndices->graphicsFamily != UINT32_MAX && queuesIndices->presentFamily != UINT32_MAX){
            return;
        }
    }

    // If not optimal selection for queues were found, select the options available
    if(queuesIndices->graphicsFamily == UINT32_MAX && queuesIndices->presentFamily == UINT32_MAX){
        queuesIndices->presentFamily = currentLessOptimalPresentSupportQueueIndice;
        queuesIndices->graphicsFamily = currentLessOptimalGraphicQueueIndice;
    }
    if(queuesIndices->transferFamily == UINT32_MAX)
        queuesIndices->transferFamily = currentLessOptimalTransferQueueIndice;

    // Panic if no queues selected
    assert(queuesIndices->presentFamily != UINT32_MAX && queuesIndices->transferFamily != UINT32_MAX && queuesIndices->graphicsFamily != UINT32_MAX);
}

void initPhysicalDeviceQueues(PhysicalDevice* pd, size_t physicalDeviceLen){

    uint32_t queueFamilyCount = 0;
    VkQueueFamilyProperties** queueFamilyProperties = malloc(sizeof(VkQueueFamilyProperties*)*physicalDeviceLen);
    VkPhysicalDeviceMemoryProperties** memProperties = malloc(sizeof(VkPhysicalDeviceMemoryProperties)*physicalDeviceLen);

    for(uint32_t i =0; i < physicalDeviceLen; i++){

        vkGetPhysicalDeviceQueueFamilyProperties(pd[i].gpu, &queueFamilyCount, NULL);

        if(queueFamilyCount > 0){
            pd->queueFamilyProperties = malloc(sizeof(VkQueueFamilyProperties) * queueFamilyCount);
            vkGetPhysicalDeviceQueueFamilyProperties(pd[i].gpu, &queueFamilyCount,  pd->queueFamilyProperties);
            queueFamilyCount = 0;
        }
        assert(queueFamilyCount >= 0);
        vkGetPhysicalDeviceMemoryProperties(pd->gpu, &pd->memProperties);
    }
}

uint32_t findMemoryType(PhysicalDevice *pd, uint32_t typeFilter, VkMemoryPropertyFlags properties) {

    uint32_t memoryCount = pd->memProperties.memoryTypeCount;

    for (uint32_t i = 0; i < memoryCount; i++) {

        if (typeFilter & (1 << i) &&
           (pd->memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    // No fallback for specified memory type not available
    printf("NO suitable memory type available for gpu %p!\n", (void*)&pd->gpu);
    exit(1);
}
#define FOR(var, start, end, step) \
for (int var = (start); var < (end); var += (step))

void initDeviceManager(VkInstance *instance, DeviceManager *dm){
        printf(stderr, "FLAG INITDEVICEMANAGER\n");
    uint32_t supportedGpuLen = 0;
    VkResult vkEnumPhysDevcResult = vkEnumeratePhysicalDevices(*instance, &supportedGpuLen, NULL);
            printf(stderr, "FLAG INITDEVICEMANAGER 2\n");
        //fflush(stdout);
    assert(supportedGpuLen >= 1);

    dm->deviceIndex = 0;
    dm->gpuIndex = 0;
    dm->phyDeviceLen = supportedGpuLen;
    dm->deviceLen = supportedGpuLen;
    dm->pDevice = malloc(sizeof(dm->pDevice) * supportedGpuLen);
    dm->lDevice = malloc(sizeof(dm->lDevice) * supportedGpuLen);

    FOR(i, 0, supportedGpuLen, 1) {
        printf(stderr, "FLAG INITDEVICEMANAGER 3\n");
        //fflush(stdout);
        initPhysicalDeviceQueues(&dm->pDevice[i], supportedGpuLen);
    }
}

void initDeviceQueues(PhysicalDevice * const pd, LogicalDevice * const ld){

    ld->displayManager = malloc(sizeof(ld->displayManager));

    float queuePriority = 1.0;

    VkDeviceQueueCreateInfo queueCreateInfoGraphics = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = pd->queueFamilyIndices.graphicsFamily,
        .queueCount = 1,
        .pQueuePriorities = &queuePriority,
        .pNext = NULL,
    };

    VkDeviceQueueCreateInfo queueCreateInfoPresentation = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = pd->queueFamilyIndices.presentFamily,
        .queueCount = 1,
        .pQueuePriorities = &queuePriority,
        .pNext = NULL,
    };

    VkDeviceQueueCreateInfo queueCreateInfoTransfer = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = pd->queueFamilyIndices.transferFamily,
        .queueCount = 1,
        .pQueuePriorities = &queuePriority,
        .pNext = NULL,
    };

    VkDeviceQueueCreateInfo *queuesCreateInfo = malloc(sizeof(VkDeviceQueueCreateInfo) * 2); // TODO: generalizar
    //queuesCreateInfo[0] = queueCreateInfoPresentation;
    queuesCreateInfo[0] = queueCreateInfoGraphics;
    queuesCreateInfo[1] = queueCreateInfoTransfer;

    VkPhysicalDeviceFeatures deviceFeatures = {VK_FALSE};
    // TODO: moverlo a una funcion o fichero para configuracion de dispositivo y dispositivo logico
    const char* logicalDeviceExtensionNames = "VK_KHR_swapchain\0"; //TODO: mover las extensiones del device a otro lado
    //Logical device
    VkDeviceCreateInfo createLogicalDeviceInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pQueueCreateInfos = queuesCreateInfo,
        .queueCreateInfoCount = 2,
        .pEnabledFeatures = &deviceFeatures,
        .ppEnabledExtensionNames = &logicalDeviceExtensionNames,
        .enabledExtensionCount = 1,
        .pNext = NULL,
    };

    if (vkCreateDevice(pd->gpu, &createLogicalDeviceInfo, NULL, &ld->device) != VK_SUCCESS) {
        printf("failed to create logical device!");
        exit(1);
    }

    vkGetDeviceQueue(ld->device, pd->queueFamilyIndices.graphicsFamily, 0, &ld->queueManager.graphicsQueue);
    vkGetDeviceQueue(ld->device, pd->queueFamilyIndices.graphicsFamily, 0, &ld->queueManager.presentQueue);
    vkGetDeviceQueue(ld->device, pd->queueFamilyIndices.graphicsFamily, 0, &ld->queueManager.transferQueue);

}

/*
void storePhysicalDevices(VkInstance* instance){
    uint32_t supported_gpu_count = 0;

    vkEnumeratePhysicalDevices(instance, &supported_gpu_count, NULL);
    assert(supported_gpu_count >= 1);
    initDeviceManager(supported_gpu_count);
    VkResult vkEnumPhysDevcResult = vkEnumeratePhysicalDevices(*instance, &supported_gpu_count, pdmm->gpu);
};*/
