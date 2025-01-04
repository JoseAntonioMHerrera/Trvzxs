#include "swapchainManager.h"
#include "image.h"
//TODO: maybe move this function  def/decl to image.h
VkImageView createImageView(GraphicSwapchain *dsm, VkDevice device, VkFormat format, VkImage image, uint32_t mipLevels, VkImageAspectFlags aspectMask) {

    VkImageViewCreateInfo viewInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = format,
        .subresourceRange.aspectMask = aspectMask,
        .subresourceRange.baseMipLevel = 0,
        .subresourceRange.levelCount = mipLevels,
        .subresourceRange.baseArrayLayer = 0,
        .subresourceRange.layerCount = 1,
    };


    VkImageView imageView;

    if (vkCreateImageView(device, &viewInfo, VK_NULL_HANDLE, &imageView) != VK_SUCCESS) {

    }




    return imageView;
}

void createFrameBuffers(GraphicSwapchain *swapManager, VkRenderPass *renderPass, VkDevice* device) {



    swapManager->framebuffersLen = swapManager->scImagesLen;
    swapManager->p_framebuffers = malloc(sizeof(VkFramebuffer) * swapManager->scImagesLen);

    for (uint32_t i = 0; i < swapManager->scImagesLen; i++) {
        VkImageView attachments[] = {
            swapManager->msaaColorRes->imageView,
            swapManager->msaaDepthRes->imageView,
            swapManager->p_scImageView[i],
            swapManager->depthImage.common->imageView,
        };

        VkFramebufferCreateInfo framebufferInfo = {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = *renderPass,
            .attachmentCount = 4, // TODO abstract attachtments creation with everything, maybe at subpass level
            .pAttachments = attachments,
            .width = swapManager->currentExtent2D.width,
            .height = swapManager->currentExtent2D.height,
            .layers = 1,
        };

        if (vkCreateFramebuffer(*device, &framebufferInfo, NULL, &swapManager->p_framebuffers[i]) != VK_SUCCESS) {
            printf("vkCreateFramebuffer error on creation!\n");
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

 void createSwapChain(PhysicalDevice *pd, LogicalDevice *ld, DisplayManager *dm, GraphicPipelineToSurface* gps, SDL_Window* window) {
        SwapChainSupportDetails *swapChainSupportDetails = (SwapChainSupportDetails*) malloc(sizeof(SwapChainSupportDetails));
        gps->swapchain->swapChainSupportDetails = swapChainSupportDetails;
        VkResult queryPhysicalDeviceSurfaceCapabilitiesResult = vkGetPhysicalDeviceSurfaceCapabilitiesKHR((*pd->gpu),
                                                                                            dm->surface,
                                                                                            &swapChainSupportDetails->capabilities);
        assert(queryPhysicalDeviceSurfaceCapabilitiesResult == VK_SUCCESS);

        // Query de los formatos de presentacion en superficie que ofrece la gpu (physicalDevice)
        swapChainSupportDetails->surfaceFormatsLen =  0;
        VkResult queryPhysicalDeviceSurfaceFormatsCountResult = vkGetPhysicalDeviceSurfaceFormatsKHR(*pd->gpu, dm->surface,
                                                                                &swapChainSupportDetails->surfaceFormatsLen, NULL);
        assert(swapChainSupportDetails->surfaceFormatsLen > 0 && queryPhysicalDeviceSurfaceFormatsCountResult == VK_SUCCESS);


        if (swapChainSupportDetails->surfaceFormatsLen > 0) {
            swapChainSupportDetails->formats = (VkSurfaceFormatKHR*) malloc(sizeof(VkSurfaceFormatKHR)* swapChainSupportDetails->surfaceFormatsLen);
            VkResult queryPhysicalDeviceSurfaceFormatsResult = vkGetPhysicalDeviceSurfaceFormatsKHR(*pd->gpu, dm->surface, &swapChainSupportDetails->surfaceFormatsLen, swapChainSupportDetails->formats);
            assert(queryPhysicalDeviceSurfaceFormatsResult == VK_SUCCESS);

        }

        swapChainSupportDetails->presentModeLen  = 0;
        vkGetPhysicalDeviceSurfacePresentModesKHR(*pd->gpu, dm->surface, &swapChainSupportDetails->presentModeLen, NULL);

        // Query present modes
        if (swapChainSupportDetails->presentModeLen != 0) {
            swapChainSupportDetails->presentModes = (VkPresentModeKHR*) malloc(sizeof(VkPresentModeKHR)* swapChainSupportDetails->presentModeLen);

            vkGetPhysicalDeviceSurfacePresentModesKHR(*pd->gpu, dm->surface, &swapChainSupportDetails->presentModeLen, swapChainSupportDetails->presentModes);

        }


        // Seleccionamos el formatos de superficie entre los disponibles que ofrece la gpu
        gps->swapchain->currentSurfaceFormat = chooseSwapSurfaceFormat(swapChainSupportDetails->formats, swapChainSupportDetails->surfaceFormatsLen);

        //Seleccionamos el modo de presentación para las imagenes a renderizar por la gpu

        VkPresentModeKHR selectedPresentationMode =  chooseSwapPresentMode(swapChainSupportDetails->presentModes, swapChainSupportDetails->presentModeLen, VK_PRESENT_MODE_MAILBOX_KHR);
        printf("SELECCIONADO EL MODO DE PRESNETACION %d\n", selectedPresentationMode);
        //Procesamos la resolución soportada por nuestra ventana y la del framebuffer de swapchain
        chooseSwapExtent2D(&gps->swapchain->currentExtent2D, &swapChainSupportDetails->capabilities, window);
        printf("SELECCIONADO EL MODO DE PRESNETACION %d\n", 3);

        // Number of images allowed in the swapchain at the same time
        gps->swapchain->scImagesLen = swapChainSupportDetails->capabilities.minImageCount + 1;

        printf("SELECCIONADO EL MODO DE PRESNETACION %d\n", 6);
        //Check if imageCount exceeds the maxImageCount of the physical device surface capabilities
        if (swapChainSupportDetails->capabilities.maxImageCount > 0 && gps->swapchain->scImagesLen >
            swapChainSupportDetails->capabilities.maxImageCount) {
            gps->swapchain->scImagesLen = swapChainSupportDetails->capabilities.maxImageCount;
        }
        printf("SELECCIONADO EL MODO DE PRESNETACION %d\n", 7);
        // Finally, we build the SwapChain info struct
        uint32_t familyIndices[] = {pd->queueFamilyIndices.graphicsFamily, pd->queueFamilyIndices.transferFamily};
        printf("SELECCIONADO EL MODO DE PRESNETACION %d\n", 8);
        VkSwapchainCreateInfoKHR creationSwapChainInfo= {
            .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .surface = dm->surface,
            .minImageCount = gps->swapchain->scImagesLen,
            .imageFormat = gps->swapchain->currentSurfaceFormat->format,
            .imageColorSpace = gps->swapchain->currentSurfaceFormat->colorSpace,
            .imageExtent = gps->swapchain->currentExtent2D,
            .imageArrayLayers = 1,
            .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, // Para renderizar directamente la imagen
            .imageSharingMode = VK_SHARING_MODE_CONCURRENT,
            .queueFamilyIndexCount = 2,
            .pQueueFamilyIndices = familyIndices,
            .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            .presentMode = selectedPresentationMode,
            .clipped = VK_TRUE,
            .oldSwapchain = VK_NULL_HANDLE,
            .preTransform = swapChainSupportDetails->capabilities.currentTransform,
        };

        printf("Debug Swap Chain Values:\n");
        printf("Surface Format: %d\n", gps->swapchain->currentSurfaceFormat->format);
        printf("Color Space: %d\n", gps->swapchain->currentSurfaceFormat->colorSpace);
        printf("Image Count: %d\n", gps->swapchain->scImagesLen);
        printf("Image Extent: %dx%d\n", gps->swapchain->currentExtent2D.width,
                                    gps->swapchain->currentExtent2D.height);

        // Also print the available formats:
        printf("Available formats:\n");
        for (uint32_t i = 0; i < swapChainSupportDetails->surfaceFormatsLen; i++) {
            printf("Format %d: format=%d, colorSpace=%d\n",
                i,
                swapChainSupportDetails->formats[i].format,
                swapChainSupportDetails->formats[i].colorSpace);
        }

        printf("Swap Chain Creation Info:\n");
        printf("Surface: %p\n", (void*)creationSwapChainInfo.surface);
        printf("Min Image Count: %d\n", creationSwapChainInfo.minImageCount);
        printf("Image Format: %d\n", creationSwapChainInfo.imageFormat);
        printf("Color Space: %d\n", creationSwapChainInfo.imageColorSpace);
        printf("Image Extent: %dx%d\n", creationSwapChainInfo.imageExtent.width, creationSwapChainInfo.imageExtent.height);
        printf("Image Array Layers: %d\n", creationSwapChainInfo.imageArrayLayers);
        printf("Image Usage: %d\n", creationSwapChainInfo.imageUsage);
        printf("Sharing Mode: %d\n", creationSwapChainInfo.imageSharingMode);
        printf("Queue Family Index Count: %d\n", creationSwapChainInfo.queueFamilyIndexCount);
        printf("Queue Family Indices: %d, %d\n", familyIndices[0], familyIndices[1]);
        printf("Present Mode: %d\n", creationSwapChainInfo.presentMode);
        //TODO fix procesamiento de seleccion de colas, se deja hardcodeado que vamos a usar dos colas distintas 0 presentacion 1 drawing
        VkResult createSwapchainResult = vkCreateSwapchainKHR(ld->device, &creationSwapChainInfo, NULL, &gps->swapchain->VkSwapchain);
        if (createSwapchainResult != VK_SUCCESS) {
            printf("Failed to create swap chain! Error code: %d\n", createSwapchainResult);

            exit(1);
        }
        printf("SELECCIONADO EL MODO DE PRESNETACION %d\n", 9);
        vkGetSwapchainImagesKHR(ld->device, gps->swapchain->VkSwapchain, &gps->swapchain->scImagesLen, NULL);
        //displaySwapCh->p_scImages = (VkImage*) malloc(sizeof(VkImage)* displaySwapCh->scImagesLen);
        printf("SELECCIONADO EL MODO DE PRESNETACION %d\n", 10);

        if(vkGetSwapchainImagesKHR(ld->device, gps->swapchain->VkSwapchain,
            &gps->swapchain->scImagesLen,
            gps->swapchain->p_scImages) != VK_SUCCESS){


            exit(1);
        }
}

 void cleanupSwapChain(LogicalDevice *ld, DisplayManager *dm) {
    for (size_t i = 0; i < dm->graphicPipeline->swapchain->framebuffersLen; i++) {
        vkDestroyFramebuffer(ld->device, dm->graphicPipeline->swapchain->p_framebuffers[i], NULL);
    }

    for (size_t i = 0; i < dm->graphicPipeline->swapchain->scImageViewLen; i++) {
        vkDestroyImageView(ld->device, dm->graphicPipeline->swapchain->p_scImageView[i], NULL);
    }

    vkDestroySwapchainKHR(ld->device, dm->graphicPipeline->swapchain->VkSwapchain, VK_NULL_HANDLE);
}

void recreateSwapChain(PhysicalDevice *pd, LogicalDevice* ld, SDL_Window *window) {
    vkDeviceWaitIdle(ld->device);
    cleanupSwapChain(ld, ld->displayManager);
    createSwapChain(pd, ld, ld->displayManager, ld->displayManager->graphicPipeline ,window);
    createMsaaColorResources(ld->device, pd, ld->displayManager->graphicPipeline->swapchain);
    createMsaaDepthResources(ld->device, pd, ld->displayManager->graphicPipeline->swapchain);
    for (uint32_t i = 0; i < ld->displayManager->graphicPipeline->swapchain->scImagesLen; i++) {
        ld->displayManager->graphicPipeline->swapchain->p_scImageView[i] = createImageView(ld->displayManager->graphicPipeline->swapchain, ld->device,
                                                            ld->displayManager->graphicPipeline->swapchain->currentSurfaceFormat->format,
                                                            ld->displayManager->graphicPipeline->swapchain->p_scImages[i], 1, VK_IMAGE_ASPECT_COLOR_BIT);
    }
    createFrameBuffers(ld->displayManager->graphicPipeline->swapchain, ld->displayManager->graphicPipeline->renderPass, &ld->device);
}
