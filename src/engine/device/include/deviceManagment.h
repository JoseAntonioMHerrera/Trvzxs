#pragma once
#include <SDL2/SDL.h>
#include "deviceManagmentTypes.h"
#include "physicalDevice.h"

typedef struct LogicalDevice LogicalDevice;
typedef struct DisplayManager DisplayManager;
typedef struct DeviceManager DeviceManager;
void getMatchingQueueIndices(PhysicalDevice *pd, LogicalDevice* ld, VkQueueFamilyProperties* properties, int numProperties, QueueFamilyIndices *queuesIndices);
void initDeviceManager(VkInstance *instance, DeviceManager* dm, DisplayManager* dispM, uint32_t maxFramesInFlight);

// VMA allocator lifecycle — create after device init, destroy before vkDestroyDevice
void createVmaAllocator(VkInstance instance, PhysicalDevice* pd, LogicalDevice* ld);
void destroyVmaAllocator(LogicalDevice* ld);
void printVmaStats(LogicalDevice* ld);
