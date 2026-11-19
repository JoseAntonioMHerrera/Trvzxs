#pragma once
#include <vulkan/vulkan.h>
#include <stddef.h>
#include "imageTypes.h"
#include "textureManagerTypes.h"

// Capacity of the swapchain image/view arrays. createSwapChain clamps the
// requested image count to this and fails if the driver creates more.
#define SC_MAX_IMAGES 16

typedef struct SwapChainSupportDetails
{
    VkSurfaceCapabilitiesKHR capabilities;
    VkSurfaceFormatKHR*      formats;
    VkPresentModeKHR*        presentModes;
    uint32_t                 presentModeLen;
    uint32_t                 surfaceFormatsLen;
} SwapChainSupportDetails;

typedef struct GraphicSwapchain
{
    // enum para ver tipo de swapchain
    VkExtent2D     currentExtent2D;
    VkExtent3D     currentExtent3D;
    VkSwapchainKHR VkSwapchain;

    uint32_t scImagesLen; // size_t TODO
    VkImage  p_scImages[SC_MAX_IMAGES];
    VkFormat swapchainImageFormat;

    // ANTI-ALIASING RESOURCES
    CommonImage* msaaColorRes;
    CommonImage* msaaDepthRes;
    // G-Buffer
    CommonImage*     gBuffer;
    CommonImage*     nBuffer;
    VkSampler        gBufferSampler; // Nearest-neighbor sampler for G-buffer reads (screen-space UV)
    DepthBufferImage depthImage;

    // Depth + occlussion resources
    Texture2D* hiZBuffer;

    uint32_t                 scImageViewLen;
    VkImageView              p_scImageView[SC_MAX_IMAGES];
    VkSurfaceFormatKHR*      currentSurfaceFormat;
    SwapChainSupportDetails* swapChainSupportDetails;
    VkSubpassDependency*     subpassDependency;
} GraphicSwapchain;
