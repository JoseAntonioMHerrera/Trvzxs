#include "deviceManagment.h"
#include <stdlib.h>
#include <assert.h>
#include <SDL.h>

#define DEFAULT_QUEUE_INDEX_VALUE UINT32_MAX

uint32_t checkMemoryTypeCache(DeviceManager* pdmm){

}

uint32_t createMemoryTypeCache(DeviceManager* pdmm){

}
// TODO ADD IS DEVICE SUITABLE

void initDeviceQueues(PhysicalDevice * const pd, LogicalDevice * const ld){

    //ld->displayManager = malloc(sizeof(ld->displayManager));

    float queuePriority = 1.0;

    VkDeviceQueueCreateInfo queueCreateInfoGraphics = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = pd->queueFamilyIndices.graphicsFamily,
        .queueCount = 1,
        .pQueuePriorities = &queuePriority,
        .pNext = NULL,
    };
    printf("LAS COLLLLAAAAAAAAAAAAAAAAAAAAAAAS\n");
    printf("Las colas: Compute %d Graphics %d Transfer %d \n", pd->queueFamilyIndices.computeFamily, pd->queueFamilyIndices.graphicsFamily, pd->queueFamilyIndices.transferFamily);

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
    VkDeviceQueueCreateInfo queueCreateInfoCompute = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = pd->queueFamilyIndices.computeFamily,
        .queueCount = 1,
        .pQueuePriorities = &queuePriority,
        .pNext = NULL,
    };

    VkDeviceQueueCreateInfo *queuesCreateInfo = malloc(sizeof(VkDeviceQueueCreateInfo) * 3); // TODO: generalizar
    //queuesCreateInfo[0] = queueCreateInfoPresentation;
    queuesCreateInfo[0] = queueCreateInfoGraphics;
    queuesCreateInfo[1] = queueCreateInfoTransfer;
    queuesCreateInfo[2] = queueCreateInfoCompute;

    VkPhysicalDeviceFeatures deviceFeatures = {
        .samplerAnisotropy = VK_TRUE,
    };
    // TODO: moverlo a una funcion o fichero para configuracion de dispositivo y dispositivo logico
    const char* logicalDeviceExtensionNames = VK_KHR_SWAPCHAIN_EXTENSION_NAME; //TODO: mover las extensiones del device a otro lado
    //Logical device
    VkDeviceCreateInfo createLogicalDeviceInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pQueueCreateInfos = queuesCreateInfo,
        .queueCreateInfoCount = 3,
        .pEnabledFeatures = &deviceFeatures,
        .ppEnabledExtensionNames = &logicalDeviceExtensionNames,
        .enabledExtensionCount = 1,
        .pNext = NULL,
    };

    if (vkCreateDevice(*pd->gpu, &createLogicalDeviceInfo, NULL, &ld->device) != VK_SUCCESS) {

        exit(1);
    }
    ld->queueManager.graphicsQueue = malloc(sizeof(*ld->queueManager.graphicsQueue));
    ld->queueManager.transferQueue = malloc(sizeof(*ld->queueManager.transferQueue));
    ld->queueManager.presentQueue = malloc(sizeof(*ld->queueManager.presentQueue));
    ld->queueManager.computeQueue = malloc(sizeof(*ld->queueManager.computeQueue));
    vkGetDeviceQueue(ld->device, pd->queueFamilyIndices.graphicsFamily, 0, ld->queueManager.graphicsQueue);
    vkGetDeviceQueue(ld->device, pd->queueFamilyIndices.presentFamily, 0, ld->queueManager.presentQueue);
    vkGetDeviceQueue(ld->device, pd->queueFamilyIndices.transferFamily, 0, ld->queueManager.transferQueue);
    vkGetDeviceQueue(ld->device, pd->queueFamilyIndices.computeFamily, 0, ld->queueManager.computeQueue);

}

void getMatchingQueueIndices(PhysicalDevice *pd, LogicalDevice* ld, VkQueueFamilyProperties* properties, int numProperties, QueueFamilyIndices *queuesIndices) {

    assert(queuesIndices);

    queuesIndices->graphicsFamily = DEFAULT_QUEUE_INDEX_VALUE;
    queuesIndices->transferFamily = DEFAULT_QUEUE_INDEX_VALUE;
    queuesIndices->presentFamily = DEFAULT_QUEUE_INDEX_VALUE;
    queuesIndices->computeFamily = DEFAULT_QUEUE_INDEX_VALUE;

    uint32_t currentLessOptimalGraphicQueueIndice = DEFAULT_QUEUE_INDEX_VALUE;
    uint32_t currentLessOptimalTransferQueueIndice = DEFAULT_QUEUE_INDEX_VALUE;
    uint32_t currentLessOptimalPresentSupportQueueIndice = DEFAULT_QUEUE_INDEX_VALUE;
    VkBool32 presentSupport = SDL_FALSE;

    for (int i = 0; i < numProperties; i++) {
        if ((properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT ) == VK_QUEUE_GRAPHICS_BIT
           && queuesIndices->graphicsFamily == DEFAULT_QUEUE_INDEX_VALUE) {

            VkResult result = vkGetPhysicalDeviceSurfaceSupportKHR(*pd->gpu, i, ld->displayManager->surface, &presentSupport);
            assert(result == VK_SUCCESS);

            if(presentSupport){

                queuesIndices->graphicsFamily = i;
                queuesIndices->presentFamily = i;
                presentSupport = VK_FALSE;
            }else{

                currentLessOptimalGraphicQueueIndice = i;
            }

        }else if((properties[i].queueFlags & VK_QUEUE_TRANSFER_BIT ) == VK_QUEUE_TRANSFER_BIT && queuesIndices->transferFamily == DEFAULT_QUEUE_INDEX_VALUE){

            currentLessOptimalTransferQueueIndice = i;

            if((properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT ) != VK_QUEUE_GRAPHICS_BIT &&
              (properties[i].queueFlags & VK_QUEUE_COMPUTE_BIT ) != VK_QUEUE_COMPUTE_BIT ) {

                queuesIndices->transferFamily =  i;
            }

        }else if(queuesIndices->presentFamily == DEFAULT_QUEUE_INDEX_VALUE){

            vkGetPhysicalDeviceSurfaceSupportKHR(*pd->gpu, i, ld->displayManager->surface, &presentSupport);

            if(presentSupport){
                currentLessOptimalPresentSupportQueueIndice = i;
                presentSupport = VK_FALSE;

            }
        }else if((properties[i].queueFlags & VK_QUEUE_COMPUTE_BIT ) == VK_QUEUE_COMPUTE_BIT && queuesIndices->computeFamily == DEFAULT_QUEUE_INDEX_VALUE){
                queuesIndices->computeFamily = i;
        }
        else if(queuesIndices->computeFamily != DEFAULT_QUEUE_INDEX_VALUE && queuesIndices->transferFamily != DEFAULT_QUEUE_INDEX_VALUE && queuesIndices->graphicsFamily != DEFAULT_QUEUE_INDEX_VALUE && queuesIndices->presentFamily != DEFAULT_QUEUE_INDEX_VALUE){
            return;
        }
    }

    // If not optimal selection for queues were found, select the options available
    if(queuesIndices->graphicsFamily == DEFAULT_QUEUE_INDEX_VALUE && queuesIndices->presentFamily == DEFAULT_QUEUE_INDEX_VALUE){
        queuesIndices->presentFamily = currentLessOptimalPresentSupportQueueIndice;
        queuesIndices->graphicsFamily = currentLessOptimalGraphicQueueIndice;
    }
    if(queuesIndices->transferFamily == DEFAULT_QUEUE_INDEX_VALUE)
        queuesIndices->transferFamily = currentLessOptimalTransferQueueIndice;


    printf("SELECCIONADAS %d (GRAPHICS) AND %d (COMPUTE)\n", queuesIndices->graphicsFamily, queuesIndices->computeFamily);

    // Panic if no queues selected
    assert(queuesIndices->presentFamily != DEFAULT_QUEUE_INDEX_VALUE && queuesIndices->computeFamily != DEFAULT_QUEUE_INDEX_VALUE && queuesIndices->transferFamily != DEFAULT_QUEUE_INDEX_VALUE && queuesIndices->graphicsFamily != DEFAULT_QUEUE_INDEX_VALUE);
}

void initPhysicalDeviceQueues(PhysicalDevice* pd){
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(*pd->gpu, &queueFamilyCount, VK_NULL_HANDLE);

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

    exit(1);
}
#define FOR(var, start, end, step) \
for (int var = (start); var < (end); var += (step))
void physicalDeviceTest(VkPhysicalDevice* gPhysicalDevice){

    VkPhysicalDeviceProperties pdProp;
    vkGetPhysicalDeviceProperties(*gPhysicalDevice, &pdProp);






}
// TODO: just test the device with a dummy operation, the name should be change to reflect this
VkBool32 isDeviceValid(VkDevice device) {
    if (device == VK_NULL_HANDLE) {

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

uint32_t rateDeviceSuitability(VkPhysicalDevice physicalDevice,
                               VkPhysicalDeviceProperties deviceProperties,
                               VkPhysicalDeviceFeatures deviceFeatures){
    int score = 0;

    if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
        score += 1000;
    }

    // Maximum possible size of textures affects graphics quality
    score += deviceProperties.limits.maxImageDimension2D;

    // Application can't function without geometry shaders
    if (!deviceFeatures.geometryShader) {
        return 0;
    }

    return score;

}

bool isDeviceSuitable(PhysicalDevice* pd, LogicalDevice* ld){

    getMatchingQueueIndices(pd, ld, pd->queueFamilyProperties, pd->queueFamilyCount, &pd->queueFamilyIndices);

    return pd->queueFamilyIndices.graphicsFamily != DEFAULT_QUEUE_INDEX_VALUE;
}
void initDeviceManager(VkInstance *instance, DeviceManager *dm, DisplayManager* dispM){

    uint32_t supportedGpuLen = 0;
    VkResult vkEnumPhysDevcResult = vkEnumeratePhysicalDevices(*instance, &supportedGpuLen, NULL);

    assert(supportedGpuLen >= 1);
    assert(vkEnumPhysDevcResult == VK_SUCCESS);


    dm->phyDeviceLen = supportedGpuLen;
    dm->deviceLen = supportedGpuLen;
    dm->pDevice = malloc(sizeof(*dm->pDevice) * supportedGpuLen);
    dm->lDevice = malloc(sizeof(*dm->lDevice) * supportedGpuLen);


    VkPhysicalDevice* pdCandidates = malloc(sizeof(VkPhysicalDevice) * supportedGpuLen);

    vkEnumPhysDevcResult = vkEnumeratePhysicalDevices(*instance, &supportedGpuLen, pdCandidates);
    assert(vkEnumPhysDevcResult == VK_SUCCESS);

    uint32_t bestCandidateIndex = 0;
    uint32_t bestCandidateFit = 0;
    VkPhysicalDeviceProperties bestCandidateProperties;
    VkPhysicalDeviceFeatures bestCandidateFeatures;

    FOR(i, 0, supportedGpuLen, 1) {



        VkPhysicalDeviceProperties deviceProperties;
        VkPhysicalDeviceFeatures deviceFeatures;
        vkGetPhysicalDeviceProperties(pdCandidates[i], &deviceProperties);
        vkGetPhysicalDeviceFeatures(pdCandidates[i], &deviceFeatures);
        uint32_t currentCandidateScore = rateDeviceSuitability(pdCandidates[i], deviceProperties, deviceFeatures);
        if(bestCandidateFit < currentCandidateScore){
            bestCandidateFit = currentCandidateScore;
            bestCandidateIndex = i;
        };
    }


        // We init the best available device
    dm->pDevice->gpu = &pdCandidates[bestCandidateIndex]; //TODO: change gpu type from VkPHysicalDEvice* to dereferenced type
    vkGetPhysicalDeviceProperties((*dm->pDevice->gpu), &(dm->pDevice->gpuProperties) );
    dm->gpuIndex = bestCandidateIndex;
     // We set the index of the logical device and gpu selected ()
    dm->deviceIndex = bestCandidateIndex;
    dm->lDevice[dm->deviceIndex] = malloc(sizeof(**dm->lDevice));

    // We instance the logical device for the candidate selected by his index (in the future, we we'll need more than one logicaldevice per gpu)

    dm->lDevice[dm->deviceIndex]->displayManager = dispM;
    initPhysicalDeviceQueues(&(dm->pDevice[dm->gpuIndex]));
    PhysicalDevice *pd = &dm->pDevice[dm->gpuIndex];
    LogicalDevice *ld = dm->lDevice[dm->deviceIndex];


    if(!isDeviceSuitable(&dm->pDevice[bestCandidateIndex], dm->lDevice[bestCandidateIndex])){

        exit(0);
    }

    //TODO: clean the rest of gpu not selected
    /*for(int i=0; i < supportedGpuLen; i++){
        if(pdCandidates[i] != dm->pDevice->gpu){
            free(pdCandidates[i]);
        }
    }*/

    //getMatchingQueueIndices(pd, ld, pd->queueFamilyProperties, pd->queueFamilyCount, &pd->queueFamilyIndices);

    initDeviceQueues(&dm->pDevice[dm->gpuIndex], dm->lDevice[dm->deviceIndex]);
}

/*
void storePhysicalDevices(VkInstance* instance){
    uint32_t supported_gpu_count = 0;

    vkEnumeratePhysicalDevices(instance, &supported_gpu_count, NULL);
    assert(supported_gpu_count >= 1);
    initDeviceManager(supported_gpu_count);
    VkResult vkEnumPhysDevcResult = vkEnumeratePhysicalDevices(*instance, &supported_gpu_count, pdmm->gpu);
};*/
