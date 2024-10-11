#pragma once
#include <vulkan/vulkan.h>
#include "stb_image.h"
#include "buffer.h"
#include <stddef.h>
#include "image.h"
#include "routeManager.h"
#include "textureManagerTypes.h"

void createTextureImageView(GraphicSwapchain* dsm, VkDevice device, Texture2D *texture, uint32_t mipLevels);
void loadTextureImage(VkDeviceSize* imageSize, stbi_uc** pixels, uint32_t *width, uint32_t *heigth);
void createTextureImage(PhysicalDevice *pd, LogicalDevice *ld, GraphicPipelineToSurface* gdp, Texture2D** texture);
void createTextureSampler(PhysicalDevice* pd, LogicalDevice *ld, Texture2D *texture, uint32_t mipLevels);
