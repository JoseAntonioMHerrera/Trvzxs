#pragma once
#include <vulkan/vulkan.h>
#include <stddef.h>
typedef struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    VkSurfaceFormatKHR* formats;
    VkPresentModeKHR* presentModes;
    size_t presentModeLen;
    size_t surfaceFormatsLen;
} SwapChainSupportDetails;


typedef struct DisplaySwapchainManager {
    // enum para ver tipo de swapchain
    VkExtent2D currentExtent2D;
    VkExtent3D currentExtent3D;
    VkSwapchainKHR swapchain;
    size_t scImagesLen; //size_t TODO
    VkImage* p_scImages;
    VkSurfaceFormatKHR *currentSurfaceFormat;
    SwapChainSupportDetails *swapChainSupportDetails;
} DisplaySwapchainManager;
