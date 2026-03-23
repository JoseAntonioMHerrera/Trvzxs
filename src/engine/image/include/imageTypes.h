#pragma once
#include <vulkan/vulkan.h>
#include "vma_bridge.h"

typedef struct CommonImage
{
    VkImage            image;
    VmaAllocation      allocation;
    VkImageView        imageView;
    VkImageView*       mipsImageViews;
    VkSampleCountFlags numSamples;
} CommonImage;

typedef struct DepthBufferImage
{
    CommonImage* common;
} DepthBufferImage;
