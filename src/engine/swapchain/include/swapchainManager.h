#pragma once
#include <SDL_vulkan.h>
#include <stddef.h>
#include <vulkan/vulkan_core.h>
#include "comparation_helper_macros.h"
#include "compilerMacros.h"
#include "swapchainManagerTypes.h"
#include "deviceManagmentTypes.h"


VkImageView createImageView(GraphicSwapchain *dsm, VkDevice device, VkFormat format, VkImage image, uint32_t mipLevels, VkImageAspectFlags aspectMask);

void createFrameBuffers(GraphicSwapchain *swapManager, VkRenderPass *renderPass, VkDevice* device);

void chooseSwapExtent2D(VkExtent2D *extent, VkSurfaceCapabilitiesKHR* capabilities, SDL_Window* window);

VkSurfaceFormatKHR* chooseSwapSurfaceFormat(VkSurfaceFormatKHR* availableFormats, uint8_t availableFormatsCount);

VkPresentModeKHR chooseSwapPresentMode(VkPresentModeKHR* availablePresentModes, uint8_t availableModesCount, VkPresentModeKHR mode);

void createSwapChain(PhysicalDevice *pd, LogicalDevice *ld, DisplayManager *displayM,SDL_Window* window);

void cleanupSwapChain(LogicalDevice *ld, DisplayManager *dm);

void recreateSwapChain(PhysicalDevice *pd, LogicalDevice* ld, SDL_Window *window);
