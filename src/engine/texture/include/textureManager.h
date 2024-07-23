#pragma once
#include <vulkan/vulkan.h>
#include "stb_image.h"
#include "buffer.h"
#include <stddef.h>
#include "image.h"
#include "routeManager.h"
#include "textureManagerTypes.h"



//#ifdef TRVZXS_TEXTURE_MANAGER_IMPLEMENTATION

void createTextureImageView(DisplaySwapchainManager* dsm, VkDevice device, Texture2D *texture);
void loadTextureImage(VkDeviceSize* imageSize, stbi_uc** pixels, uint32_t *width, uint32_t *heigth);
void createTextureImage(PhysicalDevice *pd, LogicalDevice *ld, GeometryDisplayPipelineManager* gdp, Texture2D** texture);
void createTextureSampler(PhysicalDevice* pd, LogicalDevice *ld, Texture2D *texture);

/*
void loadTextureImage(VkDeviceSize* imageSize, stbi_uc** pixels, uint32_t *width, uint32_t *heigth) {
    uint32_t texWidth, texHeight, texChannels;

    *pixels = stbi_load(PREPEND_ABSOLUTE_PATH("assets/textures/texture.jpg"), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    *imageSize = texWidth * texHeight * 4;
    *width = texWidth;
    *heigth = texHeight;

    if (!pixels) {
        printf("failed to load texture image!\n");
        exit(1);
    }
}
void createTextureImage(PhysicalDevice *pd, LogicalDevice *ld, GeometryDisplayPipelineManager* gdp, Texture2D** texture){
    printf("CREATE FLAG TEXTURE IMAGE 1\n");
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    VkDeviceSize imageSize;
    uint32_t width, height;
    stbi_uc* pixels;
    VkBuffer textureBuffer;

    *texture = (Texture2D*) malloc(sizeof(Texture2D));
    loadTextureImage(&imageSize, &pixels, &width, &height);
    printf("CREATE FLAG TEXTURE IMAGE 1\n");
    createVkBuffer(pd, ld->device, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,VK_SHARING_MODE_EXCLUSIVE,
    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    &stagingBuffer, &stagingBufferMemory);
    printf("CREATE FLAG TEXTURE IMAGE 1\n");
    void* data;
    vkMapMemory(ld->device, stagingBufferMemory, 0, imageSize, 0, &data);
        printf("CREATE FLAG TEXTURE IMAGE 1\n");
    memcpy(data, pixels, (size_t)(imageSize));
        printf("CREATE FLAG TEXTURE IMAGE 1\n");
    vkUnmapMemory(ld->device, stagingBufferMemory);
    printf("CREATE FLAG TEXTURE IMAGE 1\n");
    stbi_image_free(pixels);
    printf("CREATE FLAG TEXTURE IMAGE 1\n");
    createVkImage(ld->device, pd, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_LAYOUT_UNDEFINED,
    ((VkImageUsageFlags)(VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT)),
    ((VkMemoryPropertyFlags)VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT), texture, width, height);
    printf("CREATE FLAG TEXTURE IMAGE AFTER VK IMAGE\n");
    transitionImageLayout(ld, gdp->commandPool, (*texture)->img, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    printf("CREATE FLAG TEXTURE IMAGE AFTER TRANSITION LAYOUT\n");
    copyBufferToImage(ld, ld->device, gdp->commandPool, ld->queueManager.graphicsQueue, stagingBuffer,(*texture)->img, width, height);
    transitionImageLayout(ld, gdp->commandPool, (*texture)->img,
    VK_FORMAT_R8G8B8A8_SRGB,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    printf("CREATE FLAG TEXTURE IMAGE AFTER BUFFER TO IMAGE\n");
}


void createTextureImageView(){

}*/


//#endif
