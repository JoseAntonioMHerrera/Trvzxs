#include "image.h"
#include "swapchainManager.h"
#include "textureManagerTypes.h"

bool hasStencilComponent(VkFormat format)
{
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}
// TODO refactor this function
void transitionImageLayout(LogicalDevice* ld, VkCommandPool commandPool, VkImage image, VkFormat format,
                           VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels,
                           VkCommandBuffer commandBuffer)
{

    if (commandBuffer == VK_NULL_HANDLE)
        commandBuffer = beginSingleTimeCommands(ld, commandPool);
    VkImageMemoryBarrier barrier = {
        .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .oldLayout           = oldLayout,
        .newLayout           = newLayout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image               = image,
    };

    barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel   = 0;
    barrier.subresourceRange.levelCount     = mipLevels;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount     = 1;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

        if (hasStencilComponent(format))
        {
            barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
    }
    else
    {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    // Operations
    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage      = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage      = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask =
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        sourceStage      = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
    {
        barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        barrier.dstAccessMask = 0;

        sourceStage      = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        destinationStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    }

    vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, VK_NULL_HANDLE, 0, VK_NULL_HANDLE, 1,
                         &barrier);

    if (ld != VK_NULL_HANDLE & commandPool != VK_NULL_HANDLE)
        endSingleTimeCommands(commandBuffer, commandPool, *ld->queueManager.graphicsQueue, ld->device);
}

void createVkImage(VmaAllocator allocator, PhysicalDevice* pd, VkFormat format, VkImageTiling tiling,
                   VkImageLayout imageLayout, VkImageUsageFlags imageUsage,
                   CommonImage* commonImage, uint32_t width, uint32_t height, uint32_t mipLevels,
                   VkSampleCountFlagBits numSamples, VkSharingMode sharingMode)
{

    uint32_t queueFamilyIndexCount   = sharingMode == VK_SHARING_MODE_CONCURRENT ? 2 : 1;
    uint32_t concurrentQueueAccess[] = {pd->queueFamilyIndices.graphicsFamily, pd->queueFamilyIndices.computeFamily};
    uint32_t exclusiveQueueAccess[]  = {pd->queueFamilyIndices.graphicsFamily};

    VkImageCreateInfo imageInfo = {
        .sType                 = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType             = VK_IMAGE_TYPE_2D,
        .extent.width          = width,
        .extent.height         = height,
        .extent.depth          = 1,
        .mipLevels             = mipLevels,
        .arrayLayers           = 1,
        .format                = format,
        .tiling                = tiling,
        .initialLayout         = imageLayout,
        .usage                 = imageUsage,
        .sharingMode           = sharingMode,
        .samples               = numSamples,
        .queueFamilyIndexCount = queueFamilyIndexCount,
        .pQueueFamilyIndices   = queueFamilyIndexCount > 1 ? concurrentQueueAccess : exclusiveQueueAccess,
        .flags                 = 0,
    };

    VkResult result = vmaBridgeCreateImage(allocator, &imageInfo, VMA_BRIDGE_MEMORY_GPU_ONLY,
                                           &commonImage->image, &commonImage->allocation);
    if (result != VK_SUCCESS)
    {
        exit(1);
    }
}

VkFormat findSupportedFormat(VkPhysicalDevice* physicalDevice, VkFormat* candidates, uint32_t candidatesLen,
                             VkImageTiling tiling, VkFormatFeatureFlags features)
{
    for (uint32_t i = 0; i < candidatesLen; i++)
    {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(*physicalDevice, candidates[i], &props);
        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
        {
            return candidates[i];
        }
        else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
        {
            return candidates[i];
        }
    }

    return VK_FORMAT_UNDEFINED;
}

// Todo: una misma funcion apra encontrar los formatos de imagenes
VkFormat findDepthImageFormat(VkPhysicalDevice* physicalDevice)
{
    VkFormat candidateFormats[3] = {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT};
    return findSupportedFormat(physicalDevice, candidateFormats, 3, VK_IMAGE_TILING_OPTIMAL,
                               VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

VkFormat findGeometryBufferFormat(VkPhysicalDevice* physicalDevice)
{
    VkFormat candidateFormats[] = {VK_FORMAT_R32G32B32A32_SFLOAT};
    return findSupportedFormat(physicalDevice, candidateFormats, 3, VK_IMAGE_TILING_OPTIMAL,
                               VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT | VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT);
}

VkFormat findNormalsBufferFormat(VkPhysicalDevice* physicalDevice)
{
    VkFormat candidateFormats[2] = {VK_FORMAT_R16G16B16A16_SFLOAT, VK_FORMAT_R16G16B16_SFLOAT};
    return findSupportedFormat(physicalDevice, candidateFormats, 3, VK_IMAGE_TILING_OPTIMAL,
                               VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT | VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT);
}

void createDepthImageResources(PhysicalDevice* pd, LogicalDevice* ld, GraphicSwapchain* dispM)
{
    VkFormat format          = (findDepthImageFormat(pd->gpu));
    dispM->depthImage.common = malloc(sizeof(*dispM->depthImage.common));
    createVkImage(ld->allocator, pd, format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_LAYOUT_UNDEFINED,
                  VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                  dispM->depthImage.common, dispM->currentExtent2D.width, dispM->currentExtent2D.height, 1, 1,
                  VK_SHARING_MODE_EXCLUSIVE);
    // TODO ocl depth
    dispM->depthImage.common->imageView =
        createImageView(dispM, ld->device, format, dispM->depthImage.common->image, 1, VK_IMAGE_ASPECT_DEPTH_BIT);
}

void createGeometryBufferResources(PhysicalDevice* pd, LogicalDevice* ld, GraphicSwapchain* dispM)
{
    VkFormat format = (findGeometryBufferFormat(pd->gpu));
    dispM->gBuffer  = malloc(sizeof(*dispM->gBuffer));
    createVkImage(ld->allocator, pd, format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_LAYOUT_UNDEFINED,
                  VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                  dispM->gBuffer, dispM->currentExtent2D.width,
                  dispM->currentExtent2D.height, 1, 1, VK_SHARING_MODE_CONCURRENT);

    dispM->gBuffer->imageView =
        createImageView(dispM, ld->device, format, dispM->gBuffer->image, 1, VK_IMAGE_ASPECT_COLOR_BIT);
}

void createNormalsBufferResources(PhysicalDevice* pd, LogicalDevice* ld, GraphicSwapchain* dispM)
{
    VkFormat format = (findNormalsBufferFormat(pd->gpu));
    dispM->nBuffer  = malloc(sizeof(*dispM->nBuffer));
    createVkImage(ld->allocator, pd, format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_LAYOUT_UNDEFINED,
                  VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                  dispM->nBuffer, dispM->currentExtent2D.width,
                  dispM->currentExtent2D.height, 1, 1, VK_SHARING_MODE_CONCURRENT);

    dispM->nBuffer->imageView =
        createImageView(dispM, ld->device, format, dispM->nBuffer->image, 1, VK_IMAGE_ASPECT_COLOR_BIT);
}

void generateImageMipmaps(VkPhysicalDevice physDevice, LogicalDevice* ld, CommandPools* commandPool, VkQueue queue,
                          VkImage image, uint32_t textWidth, uint32_t textHeight, uint32_t mipLevels,
                          VkFormat imageFormat)
{
    VkCommandBuffer commandBuffer = beginSingleTimeCommands(ld, commandPool->graphicsCommandPool);
    // TODO: setear la transferQueue para ser utilizada, comprobar que vulkan permita misma cola para diferentes
    // accionees
    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(physDevice, imageFormat, &formatProperties);
    if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
    {

        exit(1);
    }
    VkImageMemoryBarrier barrier = {.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                                    .image               = image,
                                    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                                    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                                    .subresourceRange    = {.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                                                            .baseArrayLayer = 0,
                                                            .layerCount     = 1,
                                                            .levelCount     = 1,
                                                            .baseMipLevel   = 0},
                                    .oldLayout           = VK_IMAGE_LAYOUT_UNDEFINED,
                                    .newLayout           = VK_IMAGE_LAYOUT_UNDEFINED,
                                    .srcAccessMask       = 0,
                                    .dstAccessMask       = 0};

    for (uint32_t i = 1; i < mipLevels; i++)
    {
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout                     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout                     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask                 = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask                 = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL,
                             0, NULL, 1, &barrier);

        VkImageBlit blit = {
            .srcOffsets     = {[0] = {0, 0, 0}, [1] = {textWidth, textHeight, 1}},
            .srcSubresource = {.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                               .mipLevel       = i - 1,
                               .baseArrayLayer = 0,
                               .layerCount     = 1},
            .dstOffsets =
                {[0] = {0, 0, 0}, [1] = {textWidth > 1 ? textWidth / 2 : 1, textHeight > 1 ? textHeight / 2 : 1, 1}},
            .dstSubresource = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .mipLevel = i, .baseArrayLayer = 0, .layerCount = 1}};
        vkCmdBlitImage(commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image,
                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);
        barrier.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0,
                             NULL, 0, NULL, 1, &barrier);
        if (textWidth > 1)
            textWidth /= 2;
        if (textHeight > 1)
            textHeight /= 2;
    }

    barrier.subresourceRange.baseMipLevel = mipLevels - 1;
    barrier.oldLayout                     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout                     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask                 = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask                 = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0,
                         NULL, 0, NULL, 1, &barrier);

    endSingleTimeCommands(commandBuffer, commandPool->graphicsCommandPool, queue, ld->device);
}

VkSampleCountFlagBits getMaxUsableSampleCount(PhysicalDevice* phyDevice)
{

    VkSampleCountFlags counts = phyDevice->gpuProperties.limits.framebufferColorSampleCounts &
                                phyDevice->gpuProperties.limits.framebufferDepthSampleCounts;
    if (counts & VK_SAMPLE_COUNT_64_BIT)
    {
        return VK_SAMPLE_COUNT_64_BIT;
    }
    if (counts & VK_SAMPLE_COUNT_32_BIT)
    {
        return VK_SAMPLE_COUNT_32_BIT;
    }
    if (counts & VK_SAMPLE_COUNT_16_BIT)
    {
        return VK_SAMPLE_COUNT_16_BIT;
    }
    if (counts & VK_SAMPLE_COUNT_8_BIT)
    {
        return VK_SAMPLE_COUNT_8_BIT;
    }
    if (counts & VK_SAMPLE_COUNT_4_BIT)
    {
        return VK_SAMPLE_COUNT_4_BIT;
    }
    if (counts & VK_SAMPLE_COUNT_2_BIT)
    {
        return VK_SAMPLE_COUNT_2_BIT;
    }

    return VK_SAMPLE_COUNT_1_BIT;
}

void createMsaaColorResources(LogicalDevice* ld, PhysicalDevice* pd, GraphicSwapchain* dsm)
{
    dsm->msaaColorRes    = malloc(sizeof(*dsm->msaaColorRes));
    VkFormat colorFormat = dsm->currentSurfaceFormat->format;
    // dsm->msaaColorRes->numSamples = getMaxUsableSampleCount(pd);
    dsm->msaaColorRes->numSamples = VK_SAMPLE_COUNT_1_BIT;
    createVkImage(ld->allocator, pd, colorFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_LAYOUT_UNDEFINED,
                  VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                  dsm->msaaColorRes, dsm->currentExtent2D.width,
                  dsm->currentExtent2D.height, 1, dsm->msaaColorRes->numSamples, VK_SHARING_MODE_EXCLUSIVE);
    dsm->msaaColorRes->imageView =
        createImageView(dsm, ld->device, colorFormat, dsm->msaaColorRes->image, 1, VK_IMAGE_ASPECT_COLOR_BIT);
}

void createMsaaDepthResources(LogicalDevice* ld, PhysicalDevice* pd, GraphicSwapchain* dsm)
{
    dsm->msaaDepthRes    = malloc(sizeof(*dsm->msaaDepthRes));
    VkFormat depthFormat = findDepthImageFormat(pd->gpu);
    // dsm->msaaDepthRes->numSamples = getMaxUsableSampleCount(pd);
    dsm->msaaDepthRes->numSamples = VK_SAMPLE_COUNT_1_BIT;
    createVkImage(ld->allocator, pd, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_LAYOUT_UNDEFINED,
                  VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, dsm->msaaDepthRes,
                  dsm->currentExtent2D.width, dsm->currentExtent2D.height, 1, dsm->msaaDepthRes->numSamples,
                  VK_SHARING_MODE_EXCLUSIVE);
    dsm->msaaDepthRes->imageView =
        createImageView(dsm, ld->device, depthFormat, dsm->msaaDepthRes->image, 1, VK_IMAGE_ASPECT_DEPTH_BIT);
}
