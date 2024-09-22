#pragma once
#include <vulkan/vulkan.h>
#include "imageTypes.h"

typedef struct Texture2D {
    CommonImage* common;
    VkSampler sampler;
    uint32_t mipLevels;
    uint32_t textWidth;
    uint32_t textHeight;
} Texture2D;
