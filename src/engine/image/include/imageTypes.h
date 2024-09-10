#pragma once
#include <vulkan/vulkan.h>

typedef struct CommonImage {
    VkImage image;
    VkDeviceMemory imageMemory;
    VkImageView imageView;
} CommonImage;

typedef struct DepthBufferImage {
    CommonImage* common;
} DepthBufferImage;
