#pragma once
#include <vulkan/vulkan.h>

typedef struct Texture2D {
    VkImage img;
    VkDeviceMemory memory;
    VkImageView imgView;
    VkSampler sampler;
} Texture2D;
