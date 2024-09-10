#include "image.h"
#include "textureManagerTypes.h"
#include "swapchainManager.h"

bool hasStencilComponent(VkFormat format) {
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}
//TODO refactor this function
void transitionImageLayout(LogicalDevice *ld, VkCommandPool commandPool, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands(ld, commandPool);
    VkImageMemoryBarrier barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .oldLayout = oldLayout,
        .newLayout = newLayout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image,
    };

    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;


    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

        if (hasStencilComponent(format)) {
            barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
    } else {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }



    // Operations
    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }  else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    } else {
        printf("unsupported layout transition!\n");
    }

    vkCmdPipelineBarrier(
        commandBuffer,
        sourceStage, destinationStage,
        0,
        0, VK_NULL_HANDLE,
        0, VK_NULL_HANDLE,
        1, &barrier
    );

    endSingleTimeCommands(commandBuffer, commandPool, *ld->queueManager.graphicsQueue, ld->device);
}

void createVkImage(VkDevice device, PhysicalDevice *pd, VkFormat format, VkImageTiling tiling, VkImageLayout imageLayout, VkImageUsageFlags imageUsage,
     VkMemoryPropertyFlags memoryTypeNeeded, CommonImage *commonImage, uint32_t width, uint32_t height) {

    printf("CREATEVKIMAGE FLAG 1\n");
    VkImageCreateInfo imageInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .extent.width = width,
        .extent.height = height,
        .extent.depth = 1,
        .mipLevels = 1,
        .arrayLayers = 1,
        .format = format,
        .tiling = tiling,
        .initialLayout = imageLayout,
        .usage = imageUsage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .flags = 0, // Optional
    };

    printf("CREATEVKIMAGE FLAG 2\n");

    if (vkCreateImage(device, &imageInfo, VK_NULL_HANDLE, &commonImage->image) != VK_SUCCESS) {
        printf("failed to create image!\n");
        exit(1);
    }
    printf("CREATEVKIMAGE FLAG 3\n");

    // Allocating memory for the image
    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, commonImage->image, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memRequirements.size,
        .memoryTypeIndex = findMemoryType(pd, memRequirements.memoryTypeBits, memoryTypeNeeded),
    };
    printf("CREATEVKIMAGE FLAG 4\n");

    if (vkAllocateMemory(device, &allocInfo, VK_NULL_HANDLE, &commonImage->imageMemory) != VK_SUCCESS) {
        printf("failed to allocate image memory!\n");
        exit(1);
    };
    vkBindImageMemory(device, commonImage->image, commonImage->imageMemory, 0);
    printf("CREATEVKIMAGE FLAG 5\n");

}

VkFormat* findSupportedFormat(VkPhysicalDevice *physicalDevice, VkFormat* candidates, uint32_t candidatesLen, VkImageTiling tiling, VkFormatFeatureFlags features) {
    for(uint32_t i = 0; i < candidatesLen; i++){
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(*physicalDevice, candidates[i], &props);
        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
            return &candidates[i];
        } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
                return &candidates[i];
        }
    }

    return 0;
}

VkFormat* findDepthImageFormat (VkPhysicalDevice *physicalDevice) {
    VkFormat candidateFormats[3] = {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT};
    return findSupportedFormat(physicalDevice,
        candidateFormats,
        3,
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
}

void createDepthImageResources(PhysicalDevice* pd, LogicalDevice *ld, DisplaySwapchainManager* dispM) {
    VkFormat format = *(findDepthImageFormat(pd->gpu));
    dispM->depthImage.common = malloc(sizeof(*dispM->depthImage.common));
    createVkImage(ld->device, pd, format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_LAYOUT_UNDEFINED,
    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    dispM->depthImage.common, dispM->currentExtent2D.width, dispM->currentExtent2D.height);

    dispM->depthImage.common->imageView = createImageView(dispM, ld->device, format,dispM->depthImage.common->image);
}
