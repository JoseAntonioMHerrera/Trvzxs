#pragma once
#include <vulkan/vulkan.h>
#include <stddef.h>
#include "imageTypes.h"
typedef struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    VkSurfaceFormatKHR* formats;
    VkPresentModeKHR* presentModes;
    uint32_t presentModeLen;
    uint32_t surfaceFormatsLen;
} SwapChainSupportDetails;


typedef struct GraphicSwapchain {
    // enum para ver tipo de swapchain
    VkExtent2D currentExtent2D;
    VkExtent3D currentExtent3D;
    VkSwapchainKHR VkSwapchain;

    VkFramebuffer *p_framebuffers;
    size_t framebuffersLen;

    uint32_t scImagesLen; //size_t TODO
    VkImage p_scImages[4];
    VkFormat swapchainImageFormat;

    // ANTI-ALIASING RESOURCES
    CommonImage* msaaColorRes;
    CommonImage* msaaDepthRes;

    uint32_t scImageViewLen;
    VkImageView p_scImageView[4];
    DepthBufferImage depthImage;
    VkSurfaceFormatKHR *currentSurfaceFormat;
    SwapChainSupportDetails *swapChainSupportDetails;
    VkSubpassDependency *subpassDependency;
} GraphicSwapchain;
