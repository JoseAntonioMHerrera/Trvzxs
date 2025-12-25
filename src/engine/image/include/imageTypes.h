#pragma once
#include <vulkan/vulkan.h>

typedef struct CommonImage
{
    VkImage            image;
    VkDeviceMemory     imageMemory;
    VkImageView        imageView;
    VkImageView*       mipsImageViews;
    VkSampleCountFlags numSamples;
} CommonImage;

typedef struct DepthBufferImage
{
    CommonImage* common;
} DepthBufferImage;
