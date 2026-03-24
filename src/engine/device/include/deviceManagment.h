#pragma once
#include <SDL2/SDL.h>
#include "deviceManagmentTypes.h"
#include "physicalDevice.h"

typedef struct LogicalDevice LogicalDevice;
typedef struct DisplayManager DisplayManager;
typedef struct DeviceManager DeviceManager;
// Superseded by VMA (VMA_MEMORY_USAGE_AUTO handles memory type selection internally)
// uint32_t findMemoryType(PhysicalDevice *pd, uint32_t typeFilter, VkMemoryPropertyFlags properties);
void getMatchingQueueIndices(PhysicalDevice *pd, LogicalDevice* ld, VkQueueFamilyProperties* properties, int numProperties, QueueFamilyIndices *queuesIndices);
void initDeviceManager(VkInstance *instance, DeviceManager* dm, DisplayManager* dispM);

// VMA allocator lifecycle — create after device init, destroy before vkDestroyDevice
void createVmaAllocator(VkInstance instance, PhysicalDevice* pd, LogicalDevice* ld);
void destroyVmaAllocator(LogicalDevice* ld);
void printVmaStats(LogicalDevice* ld);
