#include "swapchainManager.h"
#include "textureManager.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

void loadTextureImage(VkDeviceSize* imageSize, stbi_uc** pixels, uint32_t* width, uint32_t* heigth)
{
      uint32_t texWidth, texHeight, texChannels;

      *pixels = stbi_load(PREPEND_ABSOLUTE_PATH("assets/textures/viking_room.png"), &texWidth, &texHeight, &texChannels,
                          STBI_rgb_alpha);
      assert(*pixels);
      *imageSize = (VkDeviceSize) texWidth * texHeight * 4;
      *width     = texWidth;
      *heigth    = texHeight;

      if (!pixels)
      {

            exit(1);
      }
}

// TODO move texture memory init away from this function, this is only for
// VkImage creation. Also move the sampler creation.
void createTextureImage(PhysicalDevice* pd, LogicalDevice* ld, GraphicPipelineToSurface* gdp, Texture2D** texture)
{

      VkBuffer       stagingBuffer;
      VkDeviceMemory stagingBufferMemory;
      VkDeviceSize   imageSize;
      uint32_t       width, height;
      stbi_uc*       pixels;
      VkBuffer       textureBuffer;

      (*texture) = (Texture2D*) malloc(sizeof(Texture2D));

      (*texture)->common = (CommonImage*) malloc(sizeof(CommonImage));
      loadTextureImage(&imageSize, &pixels, &width, &height);

      (*texture)->textHeight = height;
      (*texture)->textWidth  = width;
      (*texture)->mipLevels  = log2(glm_max(height, width)) + 1;
      createVkBuffer(pd, ld->device, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_SHARING_MODE_EXCLUSIVE,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer,
                     &stagingBufferMemory);

      void* data;
      vkMapMemory(ld->device, stagingBufferMemory, 0, imageSize, 0, &data);

      memcpy(data, pixels, (size_t) (imageSize));

      vkUnmapMemory(ld->device, stagingBufferMemory);

      stbi_image_free(pixels);

      createVkImage(ld->device, pd, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_LAYOUT_UNDEFINED,
                    (VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT),
                    (VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT), (*texture)->common, width, height, (*texture)->mipLevels, 1,
                    VK_SHARING_MODE_CONCURRENT);

      // TODO DELETE INDIRECTION FROM COMMAND POOLS AND VQUEUE
      transitionImageLayout(ld, (gdp->commandPool->graphicsCommandPool), (*texture)->common->image,
                            VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                            (*texture)->mipLevels, VK_NULL_HANDLE);

      copyBufferToImage(ld, ld->device, (gdp->commandPool->graphicsCommandPool), *(ld->queueManager.graphicsQueue),
                        stagingBuffer, (*texture)->common->image, width, height);

      /*transitionImageLayout(ld, (gdp->commandPool->graphicsCommandPool),
      (*texture)->common->image,
      VK_FORMAT_R8G8B8A8_SRGB,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, (*texture)->mipLevels);*/

      generateImageMipmaps((*pd->gpu), ld, gdp->commandPool, (*ld->queueManager.graphicsQueue),
                           (*texture)->common->image, (*texture)->textWidth, (*texture)->textHeight,
                           (*texture)->mipLevels, VK_FORMAT_R8G8B8A8_SRGB);

      createTextureSampler(pd, ld, *texture, (*texture)->mipLevels);
}

void createTextureImageView(GraphicSwapchain* dsm, VkDevice device, Texture2D* texture, uint32_t mipLevels)
{
      texture->common->imageView = createImageView(dsm, device, VK_FORMAT_R8G8B8A8_SRGB, texture->common->image,
                                                   mipLevels, VK_IMAGE_ASPECT_COLOR_BIT);
}

void createTextureSampler(PhysicalDevice* pd, LogicalDevice* ld, Texture2D* texture, uint32_t mipLevels)
{

      VkSamplerCreateInfo samplerInfo = {
          .sType        = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
          .magFilter    = VK_FILTER_LINEAR,
          .minFilter    = VK_FILTER_LINEAR,
          .mipmapMode   = VK_SAMPLER_MIPMAP_MODE_LINEAR,
          .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
          .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
          .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
      };

      VkPhysicalDeviceProperties properties;
      vkGetPhysicalDeviceProperties(*pd->gpu, &properties); // TODO mover esto a properties miembro
                                                            // de PhysicalDevice
      samplerInfo.maxAnisotropy           = properties.limits.maxSamplerAnisotropy;
      samplerInfo.borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
      samplerInfo.unnormalizedCoordinates = VK_FALSE;
      samplerInfo.compareEnable           = VK_FALSE;
      samplerInfo.compareOp               = VK_COMPARE_OP_ALWAYS;
      samplerInfo.mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR;
      samplerInfo.mipLodBias              = 0.0f;
      samplerInfo.minLod                  = 0;
      samplerInfo.maxLod                  = mipLevels;
      samplerInfo.anisotropyEnable        = VK_TRUE;
      samplerInfo.maxAnisotropy           = 1.0f;

      if (vkCreateSampler(ld->device, &samplerInfo, VK_NULL_HANDLE, &texture->sampler) != VK_SUCCESS)
      {
      }
}
