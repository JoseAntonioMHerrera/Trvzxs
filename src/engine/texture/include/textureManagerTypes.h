#pragma once
#include <vulkan/vulkan.h>
#include "imageTypes.h"

typedef struct Texture2D {
    CommonImage* common;
    VkSampler sampler;
} Texture2D;
