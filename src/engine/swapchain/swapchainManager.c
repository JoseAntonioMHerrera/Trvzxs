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
        };

        VkFramebufferCreateInfo framebufferInfo = {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = *renderPass,
            .attachmentCount = 3, // TODO abstract attachtments creation with everything, maybe at subpass level
            .pAttachments = attachments,
            .width = swapManager->currentExtent2D.width,
            .height = swapManager->currentExtent2D.height,
            .layers = 1,
        };

        if (vkCreateFramebuffer(*device, &framebufferInfo, NULL, &swapManager->p_framebuffers[i]) != VK_SUCCESS) {

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

 void createSwapChain(PhysicalDevice *pd, LogicalDevice *ld, DisplayManager *dm, SDL_Window* window) {
        SwapChainSupportDetails *swapChainSupportDetails = (SwapChainSupportDetails*) malloc(sizeof(SwapChainSupportDetails));
        dm->graphicPipeline->swapchain->swapChainSupportDetails = swapChainSupportDetails;
        VkResult queryPhysicalDeviceSurfaceCapabilitiesResult = vkGetPhysicalDeviceSurfaceCapabilitiesKHR((*pd->gpu),
                                                                                            dm->surface,
                                                                                            &swapChainSupportDetails->capabilities);
        //assert(queryPhysicalDeviceSurfaceCapabilitiesResult == VK_SUCCESS);

        // Query de los formatos de presentacion en superficie que ofrece la gpu (physicalDevice)
        swapChainSupportDetails->surfaceFormatsLen =  0;
        VkResult queryPhysicalDeviceSurfaceFormatsCountResult = vkGetPhysicalDeviceSurfaceFormatsKHR(*pd->gpu, dm->surface,
                                                                                &swapChainSupportDetails->surfaceFormatsLen, NULL);
        //assert(swapChainSupportDetails->surfaceFormatsLen > 0 && queryPhysicalDeviceSurfaceFormatsCountResult == VK_SUCCESS);


        if (swapChainSupportDetails->surfaceFormatsLen > 0) {
            swapChainSupportDetails->formats = (VkSurfaceFormatKHR*) malloc(sizeof(VkSurfaceFormatKHR)* swapChainSupportDetails->surfaceFormatsLen);
            VkResult queryPhysicalDeviceSurfaceFormatsResult = vkGetPhysicalDeviceSurfaceFormatsKHR(*pd->gpu, dm->surface, &swapChainSupportDetails->surfaceFormatsLen, swapChainSupportDetails->formats);
            //assert(queryPhysicalDeviceSurfaceFormatsResult == VK_SUCCESS);

        }

        swapChainSupportDetails->presentModeLen  = 0;
        vkGetPhysicalDeviceSurfacePresentModesKHR(*pd->gpu, dm->surface, &swapChainSupportDetails->presentModeLen, NULL);

        // Query present modes
        if (swapChainSupportDetails->presentModeLen != 0) {
            swapChainSupportDetails->presentModes = (VkPresentModeKHR*) malloc(sizeof(VkPresentModeKHR)* swapChainSupportDetails->presentModeLen);

            vkGetPhysicalDeviceSurfacePresentModesKHR(*pd->gpu, dm->surface, &swapChainSupportDetails->presentModeLen, swapChainSupportDetails->presentModes);

        }


        // Seleccionamos el formatos de superficie entre los disponibles que ofrece la gpu
        dm->graphicPipeline->swapchain->currentSurfaceFormat = chooseSwapSurfaceFormat(swapChainSupportDetails->formats, swapChainSupportDetails->surfaceFormatsLen);

        //Seleccionamos el modo de presentación para las imagenes a renderizar por la gpu

        VkPresentModeKHR selectedPresentationMode =  chooseSwapPresentMode(swapChainSupportDetails->presentModes, swapChainSupportDetails->presentModeLen, VK_PRESENT_MODE_MAILBOX_KHR);
        printf("SELECCIONADO EL MODO DE PRESNETACION %d\n", selectedPresentationMode);
        //Procesamos la resolución soportada por nuestra ventana y la del framebuffer de swapchain
        chooseSwapExtent2D(&dm->graphicPipeline->swapchain->currentExtent2D, &swapChainSupportDetails->capabilities, window);

        // Number of images allowed in the swapchain at the same time
        dm->graphicPipeline->swapchain->scImagesLen = swapChainSupportDetails->capabilities.minImageCount + 1;

        //Check if imageCount exceeds the maxImageCount of the physical device surface capabilities
        if (swapChainSupportDetails->capabilities.maxImageCount > 0 && dm->graphicPipeline->swapchain->scImagesLen >
            swapChainSupportDetails->capabilities.maxImageCount) {
            dm->graphicPipeline->swapchain->scImagesLen = swapChainSupportDetails->capabilities.maxImageCount;
        }

        // Finally, we build the SwapChain info struct
        uint32_t familyIndices[] = {pd->queueFamilyIndices.graphicsFamily, pd->queueFamilyIndices.transferFamily};

        VkSwapchainCreateInfoKHR creationSwapChainInfo= {
            .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .surface = dm->surface,
            .minImageCount = dm->graphicPipeline->swapchain->scImagesLen,
            .imageFormat = dm->graphicPipeline->swapchain->currentSurfaceFormat->format,
            .imageColorSpace =dm->graphicPipeline->swapchain->currentSurfaceFormat->colorSpace,
            .imageExtent = dm->graphicPipeline->swapchain->currentExtent2D,
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


        //TODO fix procesamiento de seleccion de colas, se deja hardcodeado que vamos a usar dos colas distintas 0 presentacion 1 drawing
        VkResult createSwapchainResult = vkCreateSwapchainKHR(ld->device, &creationSwapChainInfo, NULL, &dm->graphicPipeline->swapchain->VkSwapchain);
        if (createSwapchainResult != VK_SUCCESS) {


            exit(1);
        }

        vkGetSwapchainImagesKHR(ld->device, dm->graphicPipeline->swapchain->VkSwapchain, &dm->graphicPipeline->swapchain->scImagesLen, NULL);
        //displaySwapCh->p_scImages = (VkImage*) malloc(sizeof(VkImage)* displaySwapCh->scImagesLen);


        if(vkGetSwapchainImagesKHR(ld->device, dm->graphicPipeline->swapchain->VkSwapchain,
            &dm->graphicPipeline->swapchain->scImagesLen,
            dm->graphicPipeline->swapchain->p_scImages) != VK_SUCCESS){


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
    createSwapChain(pd, ld, ld->displayManager, window);
    createMsaaColorResources(ld->device, pd, ld->displayManager->graphicPipeline->swapchain);
    createMsaaDepthResources(ld->device, pd, ld->displayManager->graphicPipeline->swapchain);
    for (uint32_t i = 0; i < ld->displayManager->graphicPipeline->swapchain->scImagesLen; i++) {
        ld->displayManager->graphicPipeline->swapchain->p_scImageView[i] = createImageView(ld->displayManager->graphicPipeline->swapchain, ld->device,
                                                            ld->displayManager->graphicPipeline->swapchain->currentSurfaceFormat->format,
                                                            ld->displayManager->graphicPipeline->swapchain->p_scImages[i], 1, VK_IMAGE_ASPECT_COLOR_BIT);
    }
    createFrameBuffers(ld->displayManager->graphicPipeline->swapchain, ld->displayManager->graphicPipeline->renderPass, &ld->device);
}
