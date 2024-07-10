#pragma once
#include <SDL_vulkan.h>
#include <vulkan/vulkan_core.h>
#include "../../../utils/include/comparation_helper_macros.h"
#include "compilerMacros.h"
#include "swapchainManagerTypes.h"
#include <stddef.h>


TRVZXS_inline void createImageViews(DisplaySwapchainManager *dsm, VkDevice *d){
    dsm->scImageViewLen = dsm-> scImagesLen;
    dsm->p_scImageView = malloc(sizeof(VkImageView) * dsm->scImageViewLen);
    for (size_t i = 0; i < dsm->scImageViewLen; i++) {
        VkImageViewCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = dsm->p_scImages[i],
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = dsm->currentSurfaceFormat->format,
        .components.r = VK_COMPONENT_SWIZZLE_IDENTITY,
        .components.g = VK_COMPONENT_SWIZZLE_IDENTITY,
        .components.b = VK_COMPONENT_SWIZZLE_IDENTITY,
        .components.a = VK_COMPONENT_SWIZZLE_IDENTITY,
        .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .subresourceRange.baseMipLevel = 0,
        .subresourceRange.levelCount = 1,
        .subresourceRange.baseArrayLayer = 0,
        .subresourceRange.layerCount = 1,
        };

        if (vkCreateImageView(*d, &createInfo, NULL, &dsm->p_scImageView[i]) != VK_SUCCESS) {
            printf("failed to create image views!");
        }
    }
}

TRVZXS_inline void createGraphicsRenderPass(DisplaySwapchainManager *dsm, VkDevice *d, VkRenderPass *renderPass) {

    VkAttachmentDescription colorAttachment = {
        .format = dsm->currentSurfaceFormat->format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    };


    VkAttachmentReference colorAttachmentRef = {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };

    VkSubpassDescription subpass = {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachmentRef,
    };

    dsm->subpassDependency = (VkSubpassDependency*) malloc(sizeof(dsm->subpassDependency));
    dsm->subpassDependency->srcSubpass = VK_SUBPASS_EXTERNAL;
    dsm->subpassDependency->dstSubpass = 0;
    dsm->subpassDependency->srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dsm->subpassDependency->srcAccessMask = 0;
    dsm->subpassDependency->dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dsm->subpassDependency->dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dsm->subpassDependency->dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
    VkRenderPassCreateInfo renderPassInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &colorAttachment,
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = dsm->subpassDependency,
    };

    if (vkCreateRenderPass(*d, &renderPassInfo, NULL, renderPass) != VK_SUCCESS) {
        printf("Error creating the render pass!");
        exit(0);
    }

}

TRVZXS_inline void createFrameBuffers(DisplaySwapchainManager *swapManager, VkRenderPass *renderPass, VkDevice* device){
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

TRVZXS_inline VkExtent2D chooseSwapExtent2D(VkExtent2D *extent, VkSurfaceCapabilitiesKHR* capabilities, SDL_Window* window) {
/*    if (capabilities->currentExtent.width != UINT32_MAX) {
        //TODO add logs to all the process of setting up the vulkan instance for use
        return capabilities->currentExtent;
    } else {*/

        int width, height;
        SDL_Vulkan_GetDrawableSize(window,&width, &height);
        extent->width =CLAMP(width, capabilities->minImageExtent.width, capabilities->maxImageExtent.width);
        extent->height = CLAMP(height, capabilities->minImageExtent.height, capabilities->maxImageExtent.height);

    //}
}

// Seleccionamos de entre los formatos de superficie extraidos de las propiedades de la gpu, y seleccionamos
// el formato que soporte colores con canal alfa y componentes de 8 bits y el espacio de color SRGB
TRVZXS_inline VkSurfaceFormatKHR* chooseSwapSurfaceFormat(VkSurfaceFormatKHR* availableFormats, uint8_t availableFormatsCount) {
    for (uint8_t i=0; i < availableFormatsCount; i++) {
        if (availableFormats[i].format == VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT && availableFormats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return &availableFormats[i];
        }
    }

    return &availableFormats[0];
}

TRVZXS_inline VkPresentModeKHR chooseSwapPresentMode(VkPresentModeKHR* availablePresentModes, uint8_t availableModesCount, VkPresentModeKHR mode) {
    for (uint8_t i=0; i < availableModesCount; i++) {
        if (availablePresentModes[i] == mode) {
            return availablePresentModes[i];
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

TRVZXS_inline void createSwapChain(PhysicalDevice *pd, LogicalDevice *ld, DisplayManager *displayM, DisplaySwapchainManager * displaySwapCh, SDL_Window* window){
    SwapChainSupportDetails *swapChainSupportDetails = (SwapChainSupportDetails*) malloc(sizeof(SwapChainSupportDetails));
    displaySwapCh->swapChainSupportDetails = swapChainSupportDetails;
    VkResult queryPhysicalDeviceSurfaceCapabilitiesResult = vkGetPhysicalDeviceSurfaceCapabilitiesKHR((*pd->gpu),
                                                                                        displayM->surface,
                                                                                        &swapChainSupportDetails->capabilities);
    assert(queryPhysicalDeviceSurfaceCapabilitiesResult == VK_SUCCESS);
    printf("CREATESWAPCHAIN FLAG 0\n");
    // Query de los formatos de presentacion en superficie que ofrece la gpu (physicalDevice)
    swapChainSupportDetails->surfaceFormatsLen =  0;
    VkResult queryPhysicalDeviceSurfaceFormatsCountResult = vkGetPhysicalDeviceSurfaceFormatsKHR(*pd->gpu, displayM->surface,
                                                                            &swapChainSupportDetails->surfaceFormatsLen, NULL);
    assert(swapChainSupportDetails->surfaceFormatsLen > 0 && queryPhysicalDeviceSurfaceFormatsCountResult == VK_SUCCESS);
    printf("CREATESWAPCHAIN FLAG 1\n");

    if (swapChainSupportDetails->surfaceFormatsLen > 0) {
        swapChainSupportDetails->formats = (VkSurfaceFormatKHR*) malloc(sizeof(VkSurfaceFormatKHR)* swapChainSupportDetails->surfaceFormatsLen);
        VkResult queryPhysicalDeviceSurfaceFormatsResult = vkGetPhysicalDeviceSurfaceFormatsKHR(*pd->gpu, displayM->surface, &swapChainSupportDetails->surfaceFormatsLen, swapChainSupportDetails->formats);
        assert(queryPhysicalDeviceSurfaceFormatsResult == VK_SUCCESS);
        printf("CREATESWAPCHAIN FLAG 2\n");
    }
    printf("CREATESWAPCHAIN FLAG 3\n");
    swapChainSupportDetails->presentModeLen  = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(*pd->gpu, displayM->surface, &swapChainSupportDetails->presentModeLen, NULL);
    printf("CREATESWAPCHAIN FLAG 4\n");
    // Query present modes
    if (swapChainSupportDetails->presentModeLen != 0) {
        swapChainSupportDetails->presentModes = (VkPresentModeKHR*) malloc(sizeof(VkPresentModeKHR)* swapChainSupportDetails->presentModeLen);

        vkGetPhysicalDeviceSurfacePresentModesKHR(*pd->gpu, displayM->surface, &swapChainSupportDetails->presentModeLen, swapChainSupportDetails->presentModes);
    printf("CREATESWAPCHAIN FLAG 5\n");
    }


    // Seleccionamos el formatos de superficie entre los disponibles que ofrece la gpu
    displaySwapCh->currentSurfaceFormat = chooseSwapSurfaceFormat(swapChainSupportDetails->formats, swapChainSupportDetails->surfaceFormatsLen);
printf("CREATESWAPCHAIN FLAG 6\n");
    //Seleccionamos el modo de presentación para las imagenes a renderizar por la gpu

    VkPresentModeKHR selectedPresentationMode =  chooseSwapPresentMode(swapChainSupportDetails->presentModes, swapChainSupportDetails->presentModeLen, VK_PRESENT_MODE_MAILBOX_KHR);
printf("CREATESWAPCHAIN FLAG 7\n");
    //Procesamos la resolución soportada por nuestra ventana y la del framebuffer de swapchain
    chooseSwapExtent2D(&displaySwapCh->currentExtent2D, &swapChainSupportDetails->capabilities, window);

    // Number of images allowed in the swapchain at the same time
    displaySwapCh->scImagesLen = swapChainSupportDetails->capabilities.minImageCount + 1;

    //Check if imageCount exceeds the maxImageCount of the physical device surface capabilities
    if (swapChainSupportDetails->capabilities.maxImageCount > 0 && displaySwapCh->scImagesLen >
        swapChainSupportDetails->capabilities.maxImageCount) {
        displaySwapCh->scImagesLen = swapChainSupportDetails->capabilities.maxImageCount;
    }

    // Finally, we build the SwapChain info struct
    uint32_t familyIndices[] = {pd->queueFamilyIndices.graphicsFamily, pd->queueFamilyIndices.transferFamily};
    const
    VkSwapchainCreateInfoKHR creationSwapChainInfo= {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = displayM->surface,
        .minImageCount = displaySwapCh->scImagesLen,
        .imageFormat = displaySwapCh->currentSurfaceFormat->format,
        .imageColorSpace =displaySwapCh->currentSurfaceFormat->colorSpace,
        .imageExtent = displaySwapCh->currentExtent2D,
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
    printf("swapchain %d: %x\n", 0,ld);
    //TODO fix procesamiento de seleccion de colas, se deja hardcodeado que vamos a usar dos colas distintas 0 presentacion 1 drawing
    VkResult createSwapchainResult = vkCreateSwapchainKHR(ld->device, &creationSwapChainInfo, NULL, &displaySwapCh->swapchain);
    if (createSwapchainResult != VK_SUCCESS) {
        printf("%u", createSwapchainResult);
        printf("failed to create swap chain!\n");
        exit(1);
    }

    vkGetSwapchainImagesKHR(ld->device, displaySwapCh->swapchain, &displaySwapCh->scImagesLen, NULL);
    displaySwapCh->p_scImages = (VkImage*) malloc(sizeof(VkImage)* displaySwapCh->scImagesLen);
    if(vkGetSwapchainImagesKHR(ld->device, displaySwapCh->swapchain, &displaySwapCh->scImagesLen, displaySwapCh->p_scImages) != VK_SUCCESS){
        printf("failed to retrieve images from the swap chain!\n");
        exit(1);
    }
}

TRVZXS_inline void cleanupSwapChain(LogicalDevice *ld, DisplayManager *dm, DisplaySwapchainManager *dsm) {
    /*vkDestroyShaderModule(device, vertShaderModule, NULL);
    vkDestroyShaderModule(device, fragShaderModule, NULL);*/
    for (size_t i = 0; i < dsm->framebuffersLen; i++) {
        vkDestroyFramebuffer(ld->device, dsm->p_framebuffers[i], NULL);
    }

    for (size_t i = 0; i < dsm->scImageViewLen; i++) {
        vkDestroyImageView(ld->device, dsm->p_scImageView[i], NULL);
    }

    vkDestroySwapchainKHR(ld->device, dsm->swapchain, VK_NULL_HANDLE);
}

TRVZXS_inline void recreateSwapChain(PhysicalDevice *pd, LogicalDevice* ld, SDL_Window *window) {
    vkDeviceWaitIdle(ld->device);
    cleanupSwapChain(ld, ld->displayManager, ld->displayManager->swapChainM);
    createSwapChain(pd, ld, ld->displayManager, ld->displayManager->swapChainM, window);
    createImageViews(ld->displayManager->swapChainM, &ld->device);
    createFrameBuffers(ld->displayManager->swapChainM, ld->displayManager->geomPipeline->renderPass, &ld->device);

}
