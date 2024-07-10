#pragma once
#include <SDL2/SDL.h>
#include "deviceManagmentTypes.h"
uint32_t findMemoryType(PhysicalDevice *pd, uint32_t typeFilter, VkMemoryPropertyFlags properties);
void getMatchingQueueIndices(PhysicalDevice *pd, LogicalDevice* ld, VkQueueFamilyProperties* properties, int numProperties, QueueFamilyIndices *queuesIndices);
void initDeviceManager(VkInstance *instance, DeviceManager* dm, DisplayManager* dispM);
