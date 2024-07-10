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

    VkFramebuffer *p_framebuffers;
    size_t framebuffersLen;

    size_t scImagesLen; //size_t TODO
    VkImage* p_scImages;


    size_t scImageViewLen;
    VkImageView * p_scImageView;
    VkSurfaceFormatKHR *currentSurfaceFormat;
    SwapChainSupportDetails *swapChainSupportDetails;
    VkSubpassDependency *subpassDependency;
} DisplaySwapchainManager;
