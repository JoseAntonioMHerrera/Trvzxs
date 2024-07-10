#include "deviceManagment.h"
#include <stdlib.h>
#include <assert.h>
#include <SDL.h>
uint32_t checkMemoryTypeCache(DeviceManager* pdmm){

}

uint32_t createMemoryTypeCache(DeviceManager* pdmm){

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

    if (vkCreateDevice(*pd->gpu, &createLogicalDeviceInfo, NULL, &ld->device) != VK_SUCCESS) {
        printf("faile to create logical device!");
        exit(1);
    }

    vkGetDeviceQueue(ld->device, pd->queueFamilyIndices.graphicsFamily, 0, &ld->queueManager.graphicsQueue);
    vkGetDeviceQueue(ld->device, pd->queueFamilyIndices.graphicsFamily, 0, &ld->queueManager.presentQueue);
    vkGetDeviceQueue(ld->device, pd->queueFamilyIndices.graphicsFamily, 0, &ld->queueManager.transferQueue);

}

void getMatchingQueueIndices(PhysicalDevice *pd, LogicalDevice* ld, VkQueueFamilyProperties* properties, int numProperties, QueueFamilyIndices *queuesIndices) {

    assert(queuesIndices);

    queuesIndices->graphicsFamily = UINT32_MAX;
    queuesIndices->transferFamily = UINT32_MAX;
    queuesIndices->presentFamily = UINT32_MAX;

    uint32_t currentLessOptimalGraphicQueueIndice = UINT32_MAX;
    uint32_t currentLessOptimalTransferQueueIndice = UINT32_MAX;
    uint32_t currentLessOptimalPresentSupportQueueIndice = UINT32_MAX;
    VkBool32 presentSupport = SDL_FALSE;

    for (int i = 0; i < numProperties; i++) {
        if ((properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT ) == VK_QUEUE_GRAPHICS_BIT
           && queuesIndices->graphicsFamily == UINT32_MAX) {

            VkResult result = vkGetPhysicalDeviceSurfaceSupportKHR(*pd->gpu, i, ld->displayManager->surface, &presentSupport);
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

            vkGetPhysicalDeviceSurfaceSupportKHR(*pd->gpu, i, ld->displayManager->surface, &presentSupport);

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

void initPhysicalDeviceQueues(PhysicalDevice* pd){
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(*pd->gpu, &queueFamilyCount, VK_NULL_HANDLE);
    printf("INITPSHYSICALDEVICE QUEUES 1\n");
    if(queueFamilyCount > 0){
        pd->queueFamilyProperties = malloc(sizeof(VkQueueFamilyProperties) * queueFamilyCount);
        pd->queueFamilyCount = queueFamilyCount;
        vkGetPhysicalDeviceQueueFamilyProperties(*pd->gpu, &queueFamilyCount,  pd->queueFamilyProperties);
        queueFamilyCount = 0;
    }
    assert(queueFamilyCount >= 0);
    pd->memProperties = malloc(sizeof(VkPhysicalDeviceMemoryProperties));
    vkGetPhysicalDeviceMemoryProperties(*pd->gpu, pd->memProperties);
}

uint32_t findMemoryType(PhysicalDevice *pd, uint32_t typeFilter, VkMemoryPropertyFlags properties) {

    uint32_t memoryCount = pd->memProperties->memoryTypeCount;

    for (uint32_t i = 0; i < memoryCount; i++) {

        if (typeFilter & (1 << i) &&
           (pd->memProperties->memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    // No fallback for specified memory type not available
    printf("NO suitable memory type available for gpu %p!\n", (void*)&pd->gpu);
    exit(1);
}
#define FOR(var, start, end, step) \
for (int var = (start); var < (end); var += (step))
void physicalDeviceTest(VkPhysicalDevice* gPhysicalDevice){

    VkPhysicalDeviceProperties pdProp;
    vkGetPhysicalDeviceProperties(*gPhysicalDevice, &pdProp);

    printf("%u\n", pdProp.deviceID);
    printf("%s\n", pdProp.deviceName);
    printf("%u\n", pdProp.apiVersion);
    printf("%u\n", pdProp.driverVersion);
    printf("%u\n", pdProp.vendorID);
}

VkBool32 isDeviceValid(VkDevice device) {
    if (device == VK_NULL_HANDLE) {
        printf("NULL HANDLE NINIO\n");
        return VK_FALSE;
    }

    // Allocate a small amount of memory
    VkMemoryAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = NULL,
        .allocationSize = 4,  // Just 4 bytes
        .memoryTypeIndex = 0  // Assume the first memory type is valid
    };

    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkResult result = vkAllocateMemory(device, &allocInfo, NULL, &memory);

    if (result == VK_SUCCESS) {
        vkFreeMemory(device, memory, NULL);
        return VK_TRUE;
    } else if (result == VK_ERROR_DEVICE_LOST) {
        return VK_FALSE;
    }

    // If we get here, there was some other error, but the device might still be valid
    return VK_TRUE;
}

void initDeviceManager(VkInstance *instance, DeviceManager *dm, DisplayManager* dispM){
        printf("FLAG INITDEVICEMANAGER\n");
    uint32_t supportedGpuLen = 0;
    VkResult vkEnumPhysDevcResult = vkEnumeratePhysicalDevices(*instance, &supportedGpuLen, NULL);
    printf("FLAG INITDEVICEMANAGER 2\n");
    assert(supportedGpuLen >= 1);
    dm->deviceIndex = 0;
    dm->gpuIndex = 0;
    dm->phyDeviceLen = supportedGpuLen;
    dm->deviceLen = supportedGpuLen;
    dm->pDevice = malloc(sizeof(*dm->pDevice) * supportedGpuLen);
    dm->lDevice = malloc(sizeof(*dm->lDevice) * supportedGpuLen);
    dm->lDevice[0] = malloc(sizeof(**dm->lDevice));
    dm->lDevice[0]->displayManager = dispM;
    dm->pDevice[0].gpu = malloc(sizeof(VkPhysicalDevice));


    vkEnumPhysDevcResult = vkEnumeratePhysicalDevices(*instance, &supportedGpuLen, dm->pDevice[dm->gpuIndex].gpu);
    assert(vkEnumPhysDevcResult == VK_SUCCESS);

    FOR(i, 0, supportedGpuLen, 1) {
        printf("FLAG INITDEVICEMANAGER 3\n");
        printf("%d\n", i);

        initPhysicalDeviceQueues(&(dm->pDevice[i]));
        PhysicalDevice *pd = &dm->pDevice[dm->gpuIndex];
        LogicalDevice *ld = dm->lDevice[dm->deviceIndex];
                printf("Supported gpu len: %d\n", supportedGpuLen);
        getMatchingQueueIndices(pd, ld, pd->queueFamilyProperties, pd->queueFamilyCount, &pd->queueFamilyIndices);
        printf("%u %u %u", pd->queueFamilyIndices.graphicsFamily,pd->queueFamilyIndices.presentFamily,pd->queueFamilyIndices.transferFamily);
        initDeviceQueues(&dm->pDevice[dm->gpuIndex], dm->lDevice[dm->deviceIndex]);
    }
}

/*
void storePhysicalDevices(VkInstance* instance){
    uint32_t supported_gpu_count = 0;

    vkEnumeratePhysicalDevices(instance, &supported_gpu_count, NULL);
    assert(supported_gpu_count >= 1);
    initDeviceManager(supported_gpu_count);
    VkResult vkEnumPhysDevcResult = vkEnumeratePhysicalDevices(*instance, &supported_gpu_count, pdmm->gpu);
};*/
