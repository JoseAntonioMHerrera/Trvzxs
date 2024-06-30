#ifndef _TRVZXS_DEVICE_MANAGMENT_H_
#define _TRVZXS_DEVICE_MANAGMENT_H_
#include <vulkanHut.h>

#define GPU_INDEX uint32_t
#define DEVICE_DEFAULT_INDEX uint32_t

typedef struct {
    uint32_t graphicsFamily;
    uint32_t presentFamily;
    uint32_t transferFamily;
} QueueFamilyIndices;

typedef struct {
    uint32_t gpuLen;

    VkPhysicalDevice *gpu;
    VkDevice *device;

    VkPhysicalDeviceMemoryProperties **gpuMemProperties;

    VkQueueFamilyProperties **queueFamilyProperties;
    QueueFamilyIndices *queueFamilyIndices;

    uint32_t gpuDefaultIndex;
    uint32_t deviceDefaultIndex;
} PhysicalDeviceMemoryManager;

void storePhysicalDevices(VkInstance *instance, PhysicalDeviceMemoryManager *pdmm);
uint32_t findMemoryType(PhysicalDeviceMemoryManager *pdmm, uint32_t typeFilter, VkMemoryPropertyFlags properties) ;

void getMatchingQueueIndices(const VkQueueFamilyProperties* properties, int numProperties, QueueFamilyIndices *queuesIndices);
#endif
