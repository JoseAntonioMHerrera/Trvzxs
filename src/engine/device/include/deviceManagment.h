#pragma once
#include "../../include/vulkanHut.h"
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

void storePhysicalDevices();
uint32_t findMemoryType(PhysicalDeviceMemoryManager *pdmm, uint32_t typeFilter, VkMemoryPropertyFlags properties) ;
void getMatchingQueueIndices(const VkQueueFamilyProperties* properties, int numProperties, QueueFamilyIndices *queuesIndices);
