#pragma once

#include <vulkan/vulkan.h>
#include "deviceManagmentTypes.h"
#include "commandBuffer.h"
#include "textureManagerTypes.h"
#include "imageTypes.h"

void transitionImageLayout(LogicalDevice *ld, VkCommandPool commandPool, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels);
void createVkImage(VkDevice device, PhysicalDevice *pd, VkFormat format, VkImageTiling tiling, VkImageLayout imageLayout, VkImageUsageFlags imageUsage,
     VkMemoryPropertyFlags memoryTypeNeeded, CommonImage *commonImage, uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples);
void createDepthImageResources(PhysicalDevice* pd, LogicalDevice *ld, DisplaySwapchainManager* dispM);
VkFormat findDepthImageFormat(VkPhysicalDevice *physicalDevice);
VkFormat findSupportedFormat(VkPhysicalDevice *physicalDevice, VkFormat* candidates, uint32_t candidatesLen, VkImageTiling tiling, VkFormatFeatureFlags features);
void generateImageMipmaps(VkPhysicalDevice physDevice, LogicalDevice* ld, CommandPools* commandPool, VkQueue queue, VkImage image, uint32_t textWidth, uint32_t textHeight, uint32_t mipLevels, VkFormat imageFormat);
VkSampleCountFlagBits getMaxUsableSampleCount(PhysicalDevice* phyDevice);
void createMsaaColorResources(VkDevice device, PhysicalDevice* pd, DisplaySwapchainManager *dsm);
void createMsaaDepthResources(VkDevice device, PhysicalDevice* pd, DisplaySwapchainManager *dsm);
