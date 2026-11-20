#pragma once
#include <vulkan/vulkan.h>
#include <SDL_vulkan.h>
#include <SDL.h>
#include "deviceManagmentTypes.h"
#include "displayManager.h"

void cleanup(DeviceManager *dm, LogicalDevice *ld, DisplayManager *dispM,
             GraphicPipelineToSurface *gdp, VkDebugUtilsMessengerEXT *logCallback);
void physicalDeviceTest(VkPhysicalDevice *pd);
VkBool32 isDeviceValid(VkDevice device);
void createInstance(VkInstance **instance, SDL_Window *window);
VkDebugUtilsMessengerEXT *createDebugCallbackForVulkan(const VkInstance *instance);
