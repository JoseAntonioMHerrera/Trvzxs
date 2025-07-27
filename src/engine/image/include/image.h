#pragma once

#include <vulkan/vulkan.h>
#include "deviceManagmentTypes.h"
#include "commandBuffer.h"
#include "textureManagerTypes.h"
#include "imageTypes.h"

typedef struct LogicalDevice LogicalDevice;
typedef struct PhysicalDevice PhysicalDevice;
typedef struct CommandPools CommandPools;

void transitionImageLayout(LogicalDevice *ld, VkCommandPool commandPool, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels, VkCommandBuffer commandBuffer);
void createVkImage(VkDevice device, PhysicalDevice *pd, VkFormat format, VkImageTiling tiling, VkImageLayout imageLayout, VkImageUsageFlags imageUsage,
     VkMemoryPropertyFlags memoryTypeNeeded, CommonImage *commonImage, uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkSharingMode sharingMode);
void createDepthImageResources(PhysicalDevice* pd, LogicalDevice *ld, GraphicSwapchain* dispM);
VkFormat findDepthImageFormat(VkPhysicalDevice *physicalDevice);
VkFormat findGeometryBufferFormat(VkPhysicalDevice *physicalDevice);
VkFormat findNormalsBufferFormat(VkPhysicalDevice *physicalDevice);
VkFormat findSupportedFormat(VkPhysicalDevice *physicalDevice, VkFormat* candidates, uint32_t candidatesLen, VkImageTiling tiling, VkFormatFeatureFlags features);
void generateImageMipmaps(VkPhysicalDevice physDevice, LogicalDevice* ld, CommandPools* commandPool, VkQueue queue, VkImage image, uint32_t textWidth, uint32_t textHeight, uint32_t mipLevels, VkFormat imageFormat);
VkSampleCountFlagBits getMaxUsableSampleCount(PhysicalDevice* phyDevice);
void createMsaaColorResources(VkDevice device, PhysicalDevice* pd, GraphicSwapchain *dsm);
void createMsaaDepthResources(VkDevice device, PhysicalDevice* pd, GraphicSwapchain *dsm);
void createGeometryBufferResources(PhysicalDevice* pd, LogicalDevice *ld, GraphicSwapchain* dispM);
void createNormalsBufferResources(PhysicalDevice* pd, LogicalDevice *ld, GraphicSwapchain* dispM);
