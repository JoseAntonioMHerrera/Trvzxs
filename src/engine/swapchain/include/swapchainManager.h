#pragma once
#include <SDL_vulkan.h>
#include <vulkan/vulkan_core.h>
#include "../../device/include/deviceManagment.h"
#include "../../../utils/include/comparation_helper_macros.h"
#include "../../../include/langDefinitions/compilerMacros.h"
#include "swapchainManagerTypes.h"
#include <stddef.h>

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

    VkResult queryPhysicalDeviceSurfaceCapabilitiesResult = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(pd->gpu,
                                                                                        displayM->surface,
                                                                                        &swapChainSupportDetails->capabilities);
    assert(queryPhysicalDeviceSurfaceCapabilitiesResult == VK_SUCCESS);

    // Query de los formatos de presentacion en superficie que ofrece la gpu (physicalDevice)
    swapChainSupportDetails->surfaceFormatsLen =  0;
    VkResult queryPhysicalDeviceSurfaceFormatsCountResult = vkGetPhysicalDeviceSurfaceFormatsKHR(pd->gpu, displayM->surface, &swapChainSupportDetails->surfaceFormatsLen, NULL);
    assert(swapChainSupportDetails->surfaceFormatsLen > 0 && queryPhysicalDeviceSurfaceFormatsCountResult == VK_SUCCESS);


    if (swapChainSupportDetails->surfaceFormatsLen > 0) {
        swapChainSupportDetails->formats = (VkSurfaceFormatKHR*) malloc(sizeof(VkSurfaceFormatKHR)* swapChainSupportDetails->surfaceFormatsLen);
        VkResult queryPhysicalDeviceSurfaceFormatsResult = vkGetPhysicalDeviceSurfaceFormatsKHR(pd->gpu, displayM->surface, &swapChainSupportDetails->surfaceFormatsLen, swapChainSupportDetails->formats);
        assert(queryPhysicalDeviceSurfaceFormatsResult == VK_SUCCESS);
    }


    swapChainSupportDetails->presentModeLen  = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(pd->gpu, displayM->surface, &swapChainSupportDetails->presentModeLen, NULL);
    // Query present modes
    if (swapChainSupportDetails->presentModeLen != 0) {
        swapChainSupportDetails->presentModes = (VkPresentModeKHR*) malloc(sizeof(VkPresentModeKHR)* swapChainSupportDetails->presentModeLen);

        vkGetPhysicalDeviceSurfacePresentModesKHR(pd->gpu, displayM->surface, &swapChainSupportDetails->presentModeLen, swapChainSupportDetails->presentModes);
    }


    // Seleccionamos el formatos de superficie entre los disponibles que ofrece la gpu
    displaySwapCh->currentSurfaceFormat = chooseSwapSurfaceFormat(swapChainSupportDetails->formats, swapChainSupportDetails->surfaceFormatsLen);

    //Seleccionamos el modo de presentación para las imagenes a renderizar por la gpu

    VkPresentModeKHR selectedPresentationMode =  chooseSwapPresentMode(swapChainSupportDetails->presentModes, swapChainSupportDetails->presentModeLen, VK_PRESENT_MODE_MAILBOX_KHR);

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
        .queueFamilyIndexCount = 3,
        .pQueueFamilyIndices = &(pd->queueFamilyIndices.graphicsFamily),
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = selectedPresentationMode,
        .clipped = VK_TRUE,
        .oldSwapchain = VK_NULL_HANDLE,
        .preTransform = swapChainSupportDetails->capabilities.currentTransform,
    };

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
