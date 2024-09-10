#include "textureManager.h"
#include "swapchainManager.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

void loadTextureImage(VkDeviceSize* imageSize, stbi_uc** pixels, uint32_t *width, uint32_t *heigth) {
    uint32_t texWidth, texHeight, texChannels;
    printf("LOADTEXTURE FLAG 0\n");
    *pixels = stbi_load(PREPEND_ABSOLUTE_PATH("assets/textures/statue_text.jpg"), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    assert(*pixels);
    printf("First pixel: R:%d G:%d B:%d A:%d\n",
       pixels[0][0], pixels[0][1], pixels[0][2], pixels[0][3]);
    *imageSize = (VkDeviceSize) texWidth * texHeight * 4;
    *width = texWidth;
    *heigth = texHeight;
        printf("Texture loaded: %dx%d, %d channels, size: %lu bytes\n",
       texWidth, texHeight, texChannels, *imageSize);
    printf("LOADTEXTURE FLAG 1\n");
    if (!pixels) {
        printf("failed to load texture image!\n");
        exit(1);
    }
        printf("LOADTEXTURE FLAG 3\n");

}
void createTextureImage(PhysicalDevice *pd, LogicalDevice *ld, GeometryDisplayPipelineManager* gdp, Texture2D** texture){
    printf("CREATE FLAG TEXTURE IMAGE 0\n");
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    VkDeviceSize imageSize;
    uint32_t width, height;
    stbi_uc* pixels;
    VkBuffer textureBuffer;

    (*texture) = (Texture2D*) malloc(sizeof(Texture2D));

    (*texture)->common = (CommonImage*) malloc(sizeof(CommonImage));
    loadTextureImage(&imageSize, &pixels, &width, &height);
    printf("CREATE FLAG TEXTURE IMAGE 1\n");
    printf("%lu\n", imageSize);

    createVkBuffer(pd, ld->device, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,VK_SHARING_MODE_EXCLUSIVE,
    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    &stagingBuffer, &stagingBufferMemory);
    printf("CREATE FLAG TEXTURE IMAGE 2\n");
    void* data;
    vkMapMemory(ld->device, stagingBufferMemory, 0, imageSize, 0, &data);
        printf("CREATE FLAG TEXTURE IMAGE 3\n");
    memcpy(data, pixels, (size_t)(imageSize));
        printf("CREATE FLAG TEXTURE IMAGE 4\n");
    vkUnmapMemory(ld->device, stagingBufferMemory);
    printf("CREATE FLAG TEXTURE IMAGE 5\n");
    stbi_image_free(pixels);
    printf("CREATE FLAG TEXTURE IMAGE 6\n");

    createVkImage(ld->device, pd, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_LAYOUT_UNDEFINED,
    (VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT),
    (VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT), (*texture)->common, width, height);

    printf("CREATE FLAG TEXTURE IMAGE AFTER VK IMAGE\n");

    // TODO DELETE INDIRECTION FROM COMMAND POOLS AND VQUEUE
    transitionImageLayout(ld, (gdp->commandPool->graphicsCommandPool), (*texture)->common->image,
    VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    printf("CREATE FLAG TEXTURE IMAGE AFTER TRANSITION LAYOUT\n");

    copyBufferToImage(ld, ld->device, (gdp->commandPool->graphicsCommandPool),
    *(ld->queueManager.graphicsQueue), stagingBuffer,
    (*texture)->common->image, width, height);

    transitionImageLayout(ld, (gdp->commandPool->graphicsCommandPool),
    (*texture)->common->image,
    VK_FORMAT_R8G8B8A8_SRGB,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    printf("PRUEBA  \n");

    printf("CREATE FLAG TEXTURE IMAGE AFTER BUFFER TO IMAGE\n");
    createTextureSampler(pd, ld, *texture);

    printf("DESPUES DE CREAR TEXTURE SAMPLER\n");
}


void createTextureImageView(DisplaySwapchainManager* dsm, VkDevice device, Texture2D *texture){
    texture->common->imageView = createImageView(dsm, device, VK_FORMAT_R8G8B8A8_SRGB, texture->common->image );
    printf("TEXTURA CREADA IMAGE VIEW %p", texture->common->imageView);
}


void createTextureSampler(PhysicalDevice* pd, LogicalDevice *ld, Texture2D *texture){

    VkSamplerCreateInfo samplerInfo = {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = VK_FILTER_LINEAR,
        .minFilter = VK_FILTER_LINEAR,
    .addressModeU = VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE,
    .addressModeV = VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE,
    .addressModeW = VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE,
    };

    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(*pd->gpu, &properties); // TODO mover esto a properties miembro de PhysicalDevice
    samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;
    samplerInfo.anisotropyEnable = VK_TRUE;
        samplerInfo.maxAnisotropy = 1.0f;

    if (vkCreateSampler(ld->device, &samplerInfo, VK_NULL_HANDLE, &texture->sampler) != VK_SUCCESS) {
        printf("failed to create texture sampler!\n");
    }
}
