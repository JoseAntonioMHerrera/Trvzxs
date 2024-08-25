#include "swapchainManager.h"

VkImageView createImageView(DisplaySwapchainManager *dsm, VkDevice device, VkFormat format, VkImage image) {
    printf("CREATE IMAGE VIEW FLAG 1\n");
    VkImageViewCreateInfo viewInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = format,
        .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .subresourceRange.baseMipLevel = 0,
        .subresourceRange.levelCount = 1,
        .subresourceRange.baseArrayLayer = 0,
        .subresourceRange.layerCount = 1,
    };
    printf("CREATE IMAGE VIEW FLAG 1\n");

    VkImageView imageView;
    printf("IMAGE VIEW A CREAR: %p\n", &imageView);
    if (vkCreateImageView(device, &viewInfo, VK_NULL_HANDLE, &imageView) != VK_SUCCESS) {
        printf("failed to create texture image view!\n");
    }
    printf("IMAGE VIEW CREADA %p", &imageView);
    printf("CREATE IMAGE VIEW FLAG 1\n");

    return imageView;
}

 void createFrameBuffers(DisplaySwapchainManager *swapManager, VkRenderPass *renderPass, VkDevice* device) {
    printf("CREAMOS FRAMEBUFFERS \n");
    printf("FRAMEBUFFERS %d\n", swapManager->scImagesLen);

    swapManager->framebuffersLen = swapManager->scImagesLen;
    swapManager->p_framebuffers = malloc(sizeof(VkFramebuffer) * swapManager->scImagesLen);

    for (uint32_t i = 0; i < swapManager->scImagesLen; i++) {
        VkImageView attachments[] = {
            swapManager->p_scImageView[i]
        };

        VkFramebufferCreateInfo framebufferInfo = {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = *renderPass,
            .attachmentCount = 1,
            .pAttachments = attachments,
            .width = swapManager->currentExtent2D.width,
            .height = swapManager->currentExtent2D.height,
            .layers = 1,
        };

        if (vkCreateFramebuffer(*device, &framebufferInfo, NULL, &swapManager->p_framebuffers[i]) != VK_SUCCESS) {
            printf("Error en la creación del framebuffer con la ImageView [%d]", i);
        }
    }
}

 void chooseSwapExtent2D(VkExtent2D *extent, VkSurfaceCapabilitiesKHR* capabilities, SDL_Window* window) {
    int width, height;
    SDL_Vulkan_GetDrawableSize(window, &width, &height);
    extent->width = CLAMP(width, capabilities->minImageExtent.width, capabilities->maxImageExtent.width);
    extent->height = CLAMP(height, capabilities->minImageExtent.height, capabilities->maxImageExtent.height);
}

 VkSurfaceFormatKHR* chooseSwapSurfaceFormat(VkSurfaceFormatKHR* availableFormats, uint8_t availableFormatsCount) {
    for (uint8_t i = 0; i < availableFormatsCount; i++) {
        if (availableFormats[i].format == VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT && availableFormats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return &availableFormats[i];
        }
    }
    return &availableFormats[0];
}

 VkPresentModeKHR chooseSwapPresentMode(VkPresentModeKHR* availablePresentModes, uint8_t availableModesCount, VkPresentModeKHR mode) {
    for (uint8_t i = 0; i < availableModesCount; i++) {
        if (availablePresentModes[i] == mode) {
            return availablePresentModes[i];
        }
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

 void createSwapChain(PhysicalDevice *pd, LogicalDevice *ld, DisplayManager *displayM, DisplaySwapchainManager * displaySwapCh, SDL_Window* window) {
    // Implementation of createSwapChain function
    // ... (The rest of the function remains the same)
}

 void cleanupSwapChain(LogicalDevice *ld, DisplayManager *dm, DisplaySwapchainManager *dsm) {
    for (size_t i = 0; i < dsm->framebuffersLen; i++) {
        vkDestroyFramebuffer(ld->device, dsm->p_framebuffers[i], NULL);
    }

    for (size_t i = 0; i < dsm->scImageViewLen; i++) {
        vkDestroyImageView(ld->device, dsm->p_scImageView[i], NULL);
    }

    vkDestroySwapchainKHR(ld->device, dsm->swapchain, VK_NULL_HANDLE);
}

void recreateSwapChain(PhysicalDevice *pd, LogicalDevice* ld, SDL_Window *window) {
    vkDeviceWaitIdle(ld->device);
    cleanupSwapChain(ld, ld->displayManager, ld->displayManager->swapChainM);
    createSwapChain(pd, ld, ld->displayManager, ld->displayManager->swapChainM, window);

    for (uint32_t i = 0; i < ld->displayManager->swapChainM->scImagesLen; i++) {
        ld->displayManager->swapChainM->p_scImageView[i] = createImageView(ld->displayManager->swapChainM, ld->device,
                                                            ld->displayManager->swapChainM->currentSurfaceFormat->format,
                                                            ld->displayManager->swapChainM->p_scImages[i]);
    }
    createFrameBuffers(ld->displayManager->swapChainM, ld->displayManager->geomPipeline->renderPass, &ld->device);
}
