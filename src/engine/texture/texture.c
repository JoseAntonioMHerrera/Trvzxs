#include "swapchainManager.h"
#include "textureManager.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

void loadTextureImage(VkDeviceSize* imageSize, stbi_uc** pixels, uint32_t* width, uint32_t* heigth)
{
    uint32_t texWidth, texHeight, texChannels;

    *pixels = stbi_load(PREPEND_ASSET_ROOT("assets/textures/viking_room.png"), &texWidth, &texHeight, &texChannels,
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

    VkBuffer      stagingBuffer;
    VmaAllocation stagingAlloc;
    VkDeviceSize  imageSize;
    uint32_t      width, height;
    stbi_uc*      pixels;
    VkBuffer      textureBuffer;

    (*texture) = (Texture2D*) malloc(sizeof(Texture2D));

    (*texture)->common = (CommonImage*) malloc(sizeof(CommonImage));
    loadTextureImage(&imageSize, &pixels, &width, &height);

    (*texture)->textHeight = height;
    (*texture)->textWidth  = width;
    (*texture)->mipLevels  = log2(glm_max(height, width)) + 1;
    createVkBuffer(ld->allocator, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_SHARING_MODE_EXCLUSIVE,
                   VMA_BRIDGE_MEMORY_CPU_TO_GPU, &stagingBuffer, &stagingAlloc);

    void* data;
    vmaBridgeMapMemory(ld->allocator, stagingAlloc, &data);

    memcpy(data, pixels, (size_t) (imageSize));

    vmaBridgeUnmapMemory(ld->allocator, stagingAlloc);

    stbi_image_free(pixels);

    createVkImage(ld->allocator, pd, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_LAYOUT_UNDEFINED,
                  (VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT),
                  (*texture)->common, width, height, (*texture)->mipLevels, 1,
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

    generateImageMipmaps((*pd->gpu), ld, gdp->commandPool, (*ld->queueManager.graphicsQueue), (*texture)->common->image,
                         (*texture)->textWidth, (*texture)->textHeight, (*texture)->mipLevels, VK_FORMAT_R8G8B8A8_SRGB);

    vmaBridgeDestroyBuffer(ld->allocator, stagingBuffer, stagingAlloc);

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

void createHiZImageResources(PhysicalDevice* pd, LogicalDevice* ld, GraphicPipelineToSurface* depthGps)
{
    VkFormat          format = findDepthImageFormat(pd->gpu);
    GraphicSwapchain* gChain = depthGps->swapchain;

    // Allocate Hi-Z buffer structure
    gChain->hiZBuffer         = malloc(sizeof(*gChain->hiZBuffer));
    gChain->hiZBuffer->common = malloc(sizeof(*gChain->hiZBuffer->common));

    // Calculate dimensions and mip levels
    uint32_t width                = gChain->currentExtent2D.width;
    uint32_t height               = gChain->currentExtent2D.height;
    gChain->hiZBuffer->textWidth  = width;
    gChain->hiZBuffer->textHeight = height;

    // Calculate optimal mip levels for Hi-Z (typically log2 of max dimension)
    gChain->hiZBuffer->mipLevels = (uint32_t) floor(log2(glm_max(width, height))) + 1;

    Texture2D* hiZTexture = gChain->hiZBuffer;

    // Create the Hi-Z image with proper usage flags
    createVkImage(ld->allocator, pd, format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_LAYOUT_UNDEFINED,
                  VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
                      VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                  hiZTexture->common, width, height, hiZTexture->mipLevels, 1,
                  VK_SHARING_MODE_EXCLUSIVE);

    // Transition the entire image to depth-stencil optimal layout
    transitionImageLayout(ld, depthGps->commandPool->graphicsCommandPool, hiZTexture->common->image, format,
                          VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                          hiZTexture->mipLevels, VK_NULL_HANDLE);

    // Create main image view for the entire mip chain
    hiZTexture->common->imageView = createImageView(gChain, ld->device, format, hiZTexture->common->image,
                                                    hiZTexture->mipLevels, VK_IMAGE_ASPECT_DEPTH_BIT);

    // Allocate array for individual mip level views using existing imageView2 field
    hiZTexture->common->mipsImageViews = malloc(sizeof(VkImageView) * hiZTexture->mipLevels);

    // Create individual image views for each mip level (needed for Hi-Z updates)
    for (int32_t mipLevel = 0; mipLevel < hiZTexture->mipLevels; mipLevel++)
    {
        VkImageViewCreateInfo viewInfo = {
            .sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image                           = hiZTexture->common->image,
            .viewType                        = VK_IMAGE_VIEW_TYPE_2D,
            .format                          = format,
            .subresourceRange.aspectMask     = VK_IMAGE_ASPECT_DEPTH_BIT,
            .subresourceRange.baseMipLevel   = mipLevel,
            .subresourceRange.levelCount     = 1,
            .subresourceRange.baseArrayLayer = 0,
            .subresourceRange.layerCount     = 1,
        };

        if (vkCreateImageView(ld->device, &viewInfo, VK_NULL_HANDLE, &hiZTexture->common->mipsImageViews[mipLevel]) !=
            VK_SUCCESS)
        {
            exit(1);
        }
    }

    VkSamplerCreateInfo samplerInfo = {
        .sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter               = VK_FILTER_NEAREST,
        .minFilter               = VK_FILTER_NEAREST,
        .addressModeU            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .addressModeV            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .addressModeW            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .anisotropyEnable        = VK_FALSE,
        .maxAnisotropy           = 1.0f,
        .borderColor             = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE,
        .unnormalizedCoordinates = VK_FALSE,
        .compareEnable           = VK_FALSE,
        .compareOp               = VK_COMPARE_OP_ALWAYS,
        .mipmapMode              = VK_SAMPLER_MIPMAP_MODE_NEAREST,
        .mipLodBias              = 0.0f,
        .minLod                  = 0.0f,
        .maxLod                  = (float) hiZTexture->mipLevels,
    };

    if (vkCreateSampler(ld->device, &samplerInfo, VK_NULL_HANDLE, &hiZTexture->sampler) != VK_SUCCESS)
    {
        exit(1);
    }

}
