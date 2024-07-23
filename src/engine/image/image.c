#include "image.h"
#include "textureManagerTypes.h"

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
     VkMemoryPropertyFlags memoryTypeNeeded, Texture2D *texture, uint32_t texWidth, uint32_t texHeight) {

    printf("CREATEVKIMAGE FLAG 1\n");
    VkImageCreateInfo imageInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .extent.width = texWidth,
        .extent.height = texHeight,
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

    if (vkCreateImage(device, &imageInfo, VK_NULL_HANDLE, &texture->img) != VK_SUCCESS) {
        printf("failed to create image!\n");
        exit(1);
    }
    printf("CREATEVKIMAGE FLAG 3\n");

    // Allocating memory for the image
    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, texture->img, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memRequirements.size,
        .memoryTypeIndex = findMemoryType(pd, memRequirements.memoryTypeBits, memoryTypeNeeded),
    };
    printf("CREATEVKIMAGE FLAG 4\n");

    if (vkAllocateMemory(device, &allocInfo, VK_NULL_HANDLE, &texture->memory) != VK_SUCCESS) {
        printf("failed to allocate image memory!\n");
        exit(1);
    }
    vkBindImageMemory(device, texture->img, texture->memory, 0);
    printf("CREATEVKIMAGE FLAG 5\n");

}
