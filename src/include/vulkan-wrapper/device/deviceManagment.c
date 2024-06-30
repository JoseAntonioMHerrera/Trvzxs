
#include <deviceManagment.h>

uint32_t checkMemoryTypeCache(PhysicalDeviceMemoryManager* pdmm){

}

uint32_t createMemoryTypeCache(PhysicalDeviceMemoryManager* pdmm){

}

void getMatchingQueueIndices(const VkQueueFamilyProperties* properties, int numProperties, QueueFamilyIndices *queuesIndices) {

    assert(physicalDevicesLen >= 0);
    assert(queuesIndices);

    uint32_t currentLessOptimalGraphicQueueIndice = UINT32_MAX;
    uint32_t currentLessOptimalTransferQueueIndice = UINT32_MAX;
    uint32_t currentLessOptimalPresentSupportQueueIndice = UINT32_MAX;
    VkBool32 presentSupport = SDL_FALSE;

    for (int i = 0; i < numProperties; i++) {
        if ((properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT ) == VK_QUEUE_GRAPHICS_BIT
           && queuesIndices->graphicsFamily == UINT32_MAX) {

            VkResult result = vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevices[0], i, surface, &presentSupport);
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

            vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevices[0], i, surface, &presentSupport);

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

void initPhysicalDeviceQueues(PhysicalDeviceMemoryManager* pdmm){

    uint32_t queueFamilyCount = 0;
    VkQueueFamilyProperties** queueFamilyProperties = malloc(sizeof(VkQueueFamilyProperties*)*pdmm->gpuLen);
    VkPhysicalDeviceMemoryProperties** memProperties = malloc(sizeof(VkPhysicalDeviceMemoryProperties)*pdmm->gpuLen);

    pdmm->queueFamilyProperties = queueFamilyProperties;
    pdmm->gpuMemProperties = memProperties;

    for(uint32_t i =0; i < pdmm->gpuLen; i++){

        vkGetPhysicalDeviceQueueFamilyProperties(pdmm->gpu[i], &queueFamilyCount, NULL);

        if(queueFamilyCount > 0){
            queueFamilyProperties[i] = malloc(sizeof(VkQueueFamilyProperties) * queueFamilyCount);
            vkGetPhysicalDeviceQueueFamilyProperties(pdmm->gpu[pdmm->gpuDefaultIndex], &queueFamilyCount, queueFamilyProperties);
            queueFamilyCount = 0;
        }

        vkGetPhysicalDeviceMemoryProperties(pdmm->gpu[pdmm->gpuDefaultIndex], &memProperties);
    }
}

uint32_t findMemoryType(PhysicalDeviceMemoryManager *pdmm, uint32_t typeFilter, VkMemoryPropertyFlags properties) {

    uint32_t memoryCount = pdmm->gpuMemProperties[pdmm->gpuDefaultIndex]->memoryTypeCount;

    for (uint32_t i = 0; i < memoryCount; i++) {

        if (typeFilter & (1 << i) &&
           (pdmm->gpuMemProperties[pdmm->gpuDefaultIndex]->memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    // No fallback for specified memory type not available
    printf("NO suitable memory type available for gpu %u!\n", pdmm->gpuDefaultIndex);
    exit(1);
}

PhysicalDeviceMemoryManager* initPhysicalDeviceMemoryManager(uint32_t supportedGpuLen){

    PhysicalDeviceMemoryManager* pdmm = malloc(sizeof(PhysicalDeviceMemoryManager));
    pdmm->gpuDefaultIndex = 0;
    pdmm->gpuLen = supportedGpuLen;
    pdmm->gpu = malloc(sizeof(pdmm->gpu) * supportedGpuLen);
    pdmm->gpuMemProperties = malloc(sizeof(pdmm->gpuMemProperties) * supportedGpuLen);
    pdmm->queueFamilyProperties = malloc(sizeof(pdmm->queueFamilyProperties)* supportedGpuLen);
}

void initDevice(GPU_INDEX gpuIndex){

}



void storePhysicalDevices(){
    uint32_t supported_gpu_count = 0;

    vkEnumeratePhysicalDevices(instance, &supported_gpu_count, NULL);
    assert(supported_gpu_count >= 1);
    initPhysicalDeviceMemoryManager(supported_gpu_count);
    VkResult vkEnumPhysDevcResult = vkEnumeratePhysicalDevices(instance, &supported_gpu_count, pdmm.gpu);
};
