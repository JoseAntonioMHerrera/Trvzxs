#include "swapchainManager.h"
#include "image.h"
#include "deletionQueue.h"
#include "descriptor.h"
#include "renderGraph.h"
#include "graphicConfiguration.h"
// TODO: maybe move this function  def/decl to image.h
VkImageView createImageView(GraphicSwapchain* dsm, VkDevice device, VkFormat format, VkImage image, uint32_t mipLevels,
                            VkImageAspectFlags aspectMask)
{

    VkImageViewCreateInfo viewInfo = {
        .sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image                           = image,
        .viewType                        = VK_IMAGE_VIEW_TYPE_2D,
        .format                          = format,
        .subresourceRange.aspectMask     = aspectMask,
        .subresourceRange.baseMipLevel   = 0,
        .subresourceRange.levelCount     = mipLevels,
        .subresourceRange.baseArrayLayer = 0,
        .subresourceRange.layerCount     = 1,
    };

    VkImageView imageView;

    if (vkCreateImageView(device, &viewInfo, VK_NULL_HANDLE, &imageView) != VK_SUCCESS)
    {
    }

    return imageView;
}
void createPreDepthRenderPass(PhysicalDevice* pd, GraphicSwapchain* dsm, VkDevice device, GraphicPipelineToSurface* gdp)
{
    // 1 SAMPLE DEPTH ATTACHTMENT
    VkAttachmentDescription2 depthAttachment = {
        .sType          = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2,
        .format         = (findDepthImageFormat(pd->gpu)),
        .loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp        = VK_ATTACHMENT_STORE_OP_STORE, // Change from DONT_CARE
        .finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
        .samples        = VK_SAMPLE_COUNT_1_BIT,
    };
    // MODELS DEPTH ATTACHTMENT REF
    VkAttachmentReference2 depthAttachmentRef = {
        .sType      = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2,
        .attachment = 0,
        .layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };
    VkAttachmentReference2 depthPrePassAttachtmentsRef[] = {depthAttachmentRef};

    VkSubpassDescription2 depthPrepassSubpass = {
        .sType                   = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2,
        .pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount    = 0,
        .pDepthStencilAttachment = &depthAttachmentRef,
    };

    VkSubpassDependency2 subpassDependencies[] = {
        {
            // External -> Subpass 0
            .sType        = VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2,
            .srcSubpass   = VK_SUBPASS_EXTERNAL,
            .dstSubpass   = 0,
            .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
                            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT |
                            VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, // Add for layout
                                                                  // transition
            .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
                            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, // Add for layout
                                                               // transition
            .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT |
                             VK_ACCESS_MEMORY_WRITE_BIT, // Add for layout transition
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                             VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT |
                             VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                             VK_ACCESS_MEMORY_READ_BIT, // Add for layout transition
            .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT,
        },
    };

    gdp->attachtmentLen                     = 1;
    gdp->subpassLen                         = 1;
    gdp->dependenciesLen                    = 1;
    VkAttachmentDescription2 attatchments[] = {depthAttachment};
    VkSubpassDescription2    subpasses[]    = {depthPrepassSubpass};
    VkRenderPassCreateInfo2  renderPassInfo = {
         .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO_2,
         .attachmentCount = gdp->attachtmentLen,
         .pAttachments    = attatchments,
         .subpassCount    = gdp->subpassLen,
         .pSubpasses      = subpasses,
         .dependencyCount = gdp->dependenciesLen,
         .pDependencies   = subpassDependencies,
    };

    gdp->renderPass = malloc(sizeof(*gdp->renderPass));
    if (vkCreateRenderPass2(device, &renderPassInfo, NULL, gdp->renderPass) != VK_SUCCESS)
    {

        exit(0);
    }
}
void createRenderPass(PhysicalDevice* pd, GraphicSwapchain* dsm, VkDevice device, GraphicPipelineToSurface* gdp)
{

    // TODO make abstractions for resources attachtments and resolvers
    // COLOR ATTACHTMENT WITH MSAA ENABLED (N SAMPLES)
    VkAttachmentDescription2 colorAttachment = {
        .sType          = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2,
        .format         = (dsm->currentSurfaceFormat)->format,
        .loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout    = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .samples        = dsm->msaaColorRes->numSamples,
    };
    // MODELS COLOR ATTACHTMENT REF
    VkAttachmentReference2 colorAttachmentRef = {
        .sType      = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2,
        .attachment = 0,
        .layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };
    // 1 SAMPLE DEPTH ATTACHTMENT
    VkAttachmentDescription2 depthAttachment = {
        .sType          = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2,
        .format         = (findDepthImageFormat(pd->gpu)),
        .loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp        = VK_ATTACHMENT_STORE_OP_STORE, // Change from DONT_CARE
        .finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
        .samples        = VK_SAMPLE_COUNT_1_BIT,
    };
    // MODELS DEPTH ATTACHTMENT REF
    VkAttachmentReference2 depthAttachmentRef = {
        .sType      = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2,
        .attachment = 1,
        .layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };

    // PARTICLE COLOR REF ATTACHTMENT
    VkAttachmentReference2 particleReadColorAttachmentRef = {
        .sType      = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2,
        .attachment = 0, // Points draw directly to final color attachment
        .layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };

    // G-BUFFER (GEOMETRY AND NORMALS)
    VkAttachmentDescription2 geometryAttachment = {
        .sType          = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2,
        .format         = (findGeometryBufferFormat(pd->gpu)),
        .loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout    = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        .samples        = VK_SAMPLE_COUNT_1_BIT,
    };

    VkAttachmentReference2 geometryAttachmentRef = {
        .sType      = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2,
        .attachment = 2,
        .layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
    };

    VkAttachmentDescription2 gNormalsAttachment = {
        .sType          = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2,
        .format         = (findNormalsBufferFormat(pd->gpu)),
        .loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout    = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        .samples        = VK_SAMPLE_COUNT_1_BIT,
    };

    VkAttachmentReference2 gNormalsAttachmentRef = {
        .sType      = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2,
        .attachment = 3,
        .layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
    };

    VkAttachmentReference2 modelColorAttachtmentsRef[]   = {geometryAttachmentRef, gNormalsAttachmentRef};
    VkAttachmentReference2 lightColorAttachtmentsRef[]   = {colorAttachmentRef};
    VkAttachmentReference2 lightInputAttachmentRef[]     = {geometryAttachmentRef, gNormalsAttachmentRef};
    VkAttachmentReference2 particleColorAttachmentsRef[] = {particleReadColorAttachmentRef};
    uint32_t               modelColorAttachmentCount     = 2;
    uint32_t               lightColorAttachmentCount     = 1;
    uint32_t               lightInputAttachmentCount     = 2;
    uint32_t               particleColorAttachmentCount  = 1;

    VkSubpassDescription2 modelSubpass = {
        .sType                   = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2,
        .pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount    = modelColorAttachmentCount,
        .pColorAttachments       = modelColorAttachtmentsRef,
        .pDepthStencilAttachment = &depthAttachmentRef,
    };

    VkSubpassDescription2 lightVolumeSubpass = {
        .sType                   = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2,
        .pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount    = lightColorAttachmentCount,
        .inputAttachmentCount    = 0,
        .pColorAttachments       = lightColorAttachtmentsRef,
        .pDepthStencilAttachment = &depthAttachmentRef,
        .pInputAttachments       = NULL,
        .pResolveAttachments     = NULL,
        .pNext                   = NULL,
    };

    VkSubpassDescription2 particleSubpass = {
        .sType                   = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2,
        .pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount    = particleColorAttachmentCount,
        .pColorAttachments       = particleColorAttachmentsRef,
        .pDepthStencilAttachment = &depthAttachmentRef,
    };

    VkSubpassDependency2 subpassDependencies[] = {
        {
            // External -> Subpass 0
            .sType        = VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2,
            .srcSubpass   = VK_SUBPASS_EXTERNAL,
            .dstSubpass   = 0,
            .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
                            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT |
                            VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, // Add for layout
                                                                  // transition
            .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
                            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, // Add for layout
                                                               // transition
            .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT |
                             VK_ACCESS_MEMORY_WRITE_BIT, // Add for layout transition
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                             VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT |
                             VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                             VK_ACCESS_MEMORY_READ_BIT, // Add for layout transition
            .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT,
        },

        {
            // Subpass 0 -> Subpass 1
            .sType        = VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2,
            .srcSubpass   = 0,
            .dstSubpass   = 1,
            .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
            .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                             VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT,
        },
        {
            // Subpass 1 -> Subpass 2
            .sType        = VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2,
            .srcSubpass   = 1,
            .dstSubpass   = 2,
            .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT |
                            VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, // Add for layout
                                                                  // transition
            .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
                            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, // Add for layout
                                                               // transition
            .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT |
                             VK_ACCESS_MEMORY_WRITE_BIT, // Add for layout transition
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                             VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT |
                             VK_ACCESS_MEMORY_READ_BIT, // Add for layout transition
            .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT,
        },
    };

    gdp->attachtmentLen                     = 4;
    gdp->subpassLen                         = 3;
    gdp->dependenciesLen                    = 3;
    VkAttachmentDescription2 attatchments[] = {colorAttachment, depthAttachment, geometryAttachment,
                                               gNormalsAttachment};
    VkSubpassDescription2    subpasses[]    = {modelSubpass, lightVolumeSubpass, particleSubpass};
    VkRenderPassCreateInfo2  renderPassInfo = {
         .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO_2,
         .attachmentCount = gdp->attachtmentLen,
         .pAttachments    = attatchments,
         .subpassCount    = gdp->subpassLen,
         .pSubpasses      = subpasses,
         .dependencyCount = gdp->dependenciesLen,
         .pDependencies   = subpassDependencies,
    };

    gdp->renderPass = malloc(sizeof(*gdp->renderPass));
    if (vkCreateRenderPass2(device, &renderPassInfo, NULL, gdp->renderPass) != VK_SUCCESS)
    {

        exit(0);
    }
}

void createFrameBuffers(GraphicSwapchain* swapManager, VkRenderPass* renderPass, VkDevice* device)
{

    swapManager->framebuffersLen = swapManager->scImagesLen;
    swapManager->p_framebuffers  = malloc(sizeof(VkFramebuffer) * swapManager->scImagesLen);

    for (uint32_t i = 0; i < swapManager->scImagesLen; i++)
    {
        VkImageView attachments[] = {
            swapManager->p_scImageView[i],
            /*            swapManager->msaaColorRes->imageView,
                        swapManager->msaaDepthRes->imageView,*/

            swapManager->depthImage.common->imageView,
            swapManager->gBuffer->imageView,
            swapManager->nBuffer->imageView,
        };

        uint32_t attachmentsCount = 4;

        VkFramebufferCreateInfo framebufferInfo = {
            .sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass      = *renderPass,
            .attachmentCount = attachmentsCount, // TODO abstract attachtments creation
                                                 // with everything, maybe at subpass level
            .pAttachments = attachments,
            .width        = swapManager->currentExtent2D.width,
            .height       = swapManager->currentExtent2D.height,
            .layers       = 1,
        };

        if (vkCreateFramebuffer(*device, &framebufferInfo, NULL, &swapManager->p_framebuffers[i]) != VK_SUCCESS)
        {
        }
    }
}

void chooseSwapExtent2D(VkExtent2D* extent, VkSurfaceCapabilitiesKHR* capabilities, SDL_Window* window)
{
    int width, height;
    SDL_Vulkan_GetDrawableSize(window, &width, &height);
    fprintf(stderr, "[swapchain] SDL drawable size: %d x %d\n", width, height);

    // Enforce the configured resolution so profiling numbers are reproducible
    width  = WINDOW_WIDTH;
    height = WINDOW_HEIGHT;

    extent->width  = CLAMP(width, capabilities->minImageExtent.width, capabilities->maxImageExtent.width);
    extent->height = CLAMP(height, capabilities->minImageExtent.height, capabilities->maxImageExtent.height);
    fprintf(stderr, "[swapchain] Enforced extent: %u x %u\n", extent->width, extent->height);
}

VkSurfaceFormatKHR* chooseSwapSurfaceFormat(VkSurfaceFormatKHR* availableFormats, uint8_t availableFormatsCount)
{
    for (uint8_t i = 0; i < availableFormatsCount; i++)
    {
        if (availableFormats[i].format == VK_FORMAT_B8G8R8A8_SRGB &&
            availableFormats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            return &availableFormats[i];
        }
    }
    return &availableFormats[0];
}

static const char* presentModeName(VkPresentModeKHR m)
{
    switch (m)
    {
    case VK_PRESENT_MODE_IMMEDIATE_KHR:                 return "IMMEDIATE (no vsync, tearing)";
    case VK_PRESENT_MODE_MAILBOX_KHR:                   return "MAILBOX (no vsync, no tearing)";
    case VK_PRESENT_MODE_FIFO_KHR:                      return "FIFO (vsync)";
    case VK_PRESENT_MODE_FIFO_RELAXED_KHR:              return "FIFO_RELAXED";
    case VK_PRESENT_MODE_SHARED_DEMAND_REFRESH_KHR:     return "SHARED_DEMAND_REFRESH";
    case VK_PRESENT_MODE_SHARED_CONTINUOUS_REFRESH_KHR: return "SHARED_CONTINUOUS_REFRESH";
    default:                                            return "unknown";
    }
}

VkPresentModeKHR chooseSwapPresentMode(VkPresentModeKHR* availablePresentModes, uint8_t availableModesCount,
                                       VkPresentModeKHR mode)
{
    printf("[swapchain] requested present mode: %s\n", presentModeName(mode));
    printf("[swapchain] surface advertises %u present mode(s):\n", availableModesCount);
    for (uint8_t i = 0; i < availableModesCount; i++)
    {
        printf("              [%u] %s\n", i, presentModeName(availablePresentModes[i]));
    }
    for (uint8_t i = 0; i < availableModesCount; i++)
    {
        if (availablePresentModes[i] == mode)
        {
            printf("[swapchain] GRANTED: %s\n", presentModeName(availablePresentModes[i]));
            fflush(stdout);
            return availablePresentModes[i];
        }
    }
    printf("[swapchain] FALLBACK to %s — requested mode not supported by surface\n",
           presentModeName(VK_PRESENT_MODE_FIFO_KHR));
    fflush(stdout);
    return VK_PRESENT_MODE_FIFO_KHR;
}

void createSwapChain(PhysicalDevice* pd, LogicalDevice* ld, DisplayManager* dm, GraphicPipelineToSurface* gps,
                     SDL_Window* window)
{
    SwapChainSupportDetails* swapChainSupportDetails =
        (SwapChainSupportDetails*) malloc(sizeof(SwapChainSupportDetails));
    gps->swapchain->swapChainSupportDetails = swapChainSupportDetails;
    VkResult queryPhysicalDeviceSurfaceCapabilitiesResult =
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR((*pd->gpu), dm->surface, &swapChainSupportDetails->capabilities);
    assert(queryPhysicalDeviceSurfaceCapabilitiesResult == VK_SUCCESS);

    // Query de los formatos de presentacion en superficie que ofrece la gpu
    // (physicalDevice)
    swapChainSupportDetails->surfaceFormatsLen = 0;
    VkResult queryPhysicalDeviceSurfaceFormatsCountResult =
        vkGetPhysicalDeviceSurfaceFormatsKHR(*pd->gpu, dm->surface, &swapChainSupportDetails->surfaceFormatsLen, NULL);
    assert(swapChainSupportDetails->surfaceFormatsLen > 0 &&
           queryPhysicalDeviceSurfaceFormatsCountResult == VK_SUCCESS);

    if (swapChainSupportDetails->surfaceFormatsLen > 0)
    {
        swapChainSupportDetails->formats =
            (VkSurfaceFormatKHR*) malloc(sizeof(VkSurfaceFormatKHR) * swapChainSupportDetails->surfaceFormatsLen);
        VkResult queryPhysicalDeviceSurfaceFormatsResult = vkGetPhysicalDeviceSurfaceFormatsKHR(
            *pd->gpu, dm->surface, &swapChainSupportDetails->surfaceFormatsLen, swapChainSupportDetails->formats);
        assert(queryPhysicalDeviceSurfaceFormatsResult == VK_SUCCESS);
    }

    swapChainSupportDetails->presentModeLen = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(*pd->gpu, dm->surface, &swapChainSupportDetails->presentModeLen, NULL);

    // Query present modes
    if (swapChainSupportDetails->presentModeLen != 0)
    {
        swapChainSupportDetails->presentModes =
            (VkPresentModeKHR*) malloc(sizeof(VkPresentModeKHR) * swapChainSupportDetails->presentModeLen);

        vkGetPhysicalDeviceSurfacePresentModesKHR(*pd->gpu, dm->surface, &swapChainSupportDetails->presentModeLen,
                                                  swapChainSupportDetails->presentModes);
    }

    // Seleccionamos el formatos de superficie entre los disponibles que
    // ofrece la gpu
    gps->swapchain->currentSurfaceFormat =
        chooseSwapSurfaceFormat(swapChainSupportDetails->formats, swapChainSupportDetails->surfaceFormatsLen);

    // Seleccionamos el modo de presentación para las imagenes a renderizar
    // por la gpu

    VkPresentModeKHR selectedPresentationMode = chooseSwapPresentMode(
        swapChainSupportDetails->presentModes, swapChainSupportDetails->presentModeLen, VK_PRESENT_MODE_IMMEDIATE_KHR);

    // Procesamos la resolución soportada por nuestra ventana y la del
    // framebuffer de swapchain
    chooseSwapExtent2D(&gps->swapchain->currentExtent2D, &swapChainSupportDetails->capabilities, window);

    // Number of images allowed in the swapchain at the same time
    gps->swapchain->scImagesLen = swapChainSupportDetails->capabilities.minImageCount + 1;

    // Check if imageCount exceeds the maxImageCount of the physical device
    // surface capabilities
    if (swapChainSupportDetails->capabilities.maxImageCount > 0 &&
        gps->swapchain->scImagesLen > swapChainSupportDetails->capabilities.maxImageCount)
    {
        gps->swapchain->scImagesLen = swapChainSupportDetails->capabilities.maxImageCount;
    }

    // Finally, we build the SwapChain info struct
    uint32_t familyIndices[] = {pd->queueFamilyIndices.graphicsFamily, pd->queueFamilyIndices.transferFamily};
    VkSwapchainCreateInfoKHR creationSwapChainInfo = {
        .sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface          = dm->surface,
        .minImageCount    = gps->swapchain->scImagesLen,
        .imageFormat      = gps->swapchain->currentSurfaceFormat->format,
        .imageColorSpace  = gps->swapchain->currentSurfaceFormat->colorSpace,
        .imageExtent      = gps->swapchain->currentExtent2D,
        .imageArrayLayers = 1,
        .imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, // Para renderizar
                                                                 // directamente la imagen
        .imageSharingMode      = VK_SHARING_MODE_CONCURRENT,
        .queueFamilyIndexCount = 2,
        .pQueueFamilyIndices   = familyIndices,
        .compositeAlpha        = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode           = selectedPresentationMode,
        .clipped               = VK_TRUE,
        .oldSwapchain          = VK_NULL_HANDLE,
        .preTransform          = swapChainSupportDetails->capabilities.currentTransform,
    };

    // TODO fix procesamiento de seleccion de colas, se deja hardcodeado que
    // vamos a usar dos colas distintas 0 presentacion 1 drawing
    VkResult createSwapchainResult =
        vkCreateSwapchainKHR(ld->device, &creationSwapChainInfo, NULL, &gps->swapchain->VkSwapchain);
    if (createSwapchainResult != VK_SUCCESS)
    {

        exit(1);
    }
    vkGetSwapchainImagesKHR(ld->device, gps->swapchain->VkSwapchain, &gps->swapchain->scImagesLen, NULL);
    // displaySwapCh->p_scImages = (VkImage*) malloc(sizeof(VkImage)*
    // displaySwapCh->scImagesLen);

    if (vkGetSwapchainImagesKHR(ld->device, gps->swapchain->VkSwapchain, &gps->swapchain->scImagesLen,
                                gps->swapchain->p_scImages) != VK_SUCCESS)
    {

        exit(1);
    }
}

void cleanupSwapChain(LogicalDevice* ld, DisplayManager* dm)
{
    GraphicSwapchain* sc = dm->graphicPipeline->swapchain;
    DeletionQueue*    dq = &ld->deletionQueue;

    // Framebuffers reference image views — destroy first.
    for (size_t i = 0; i < sc->framebuffersLen; i++)
        vkDestroyFramebuffer(ld->device, sc->p_framebuffers[i], NULL);
    sc->framebuffersLen = 0;

    // Swapchain presentation image views
    for (size_t i = 0; i < sc->scImageViewLen; i++)
        deferDestroyImageView(dq, sc->p_scImageView[i]);

    // G-buffer sampler (shared by gBuffer + nBuffer reads)
    if (sc->gBufferSampler != VK_NULL_HANDLE)
        vkDestroySampler(ld->device, sc->gBufferSampler, NULL);

    // Destroy image views for depth/gbuffer/normals (not deferred — they're
    // recreated immediately in recreateSwapChain, and the underlying images
    // are deferred below).
    if (sc->depthImage.common && sc->depthImage.common->imageView != VK_NULL_HANDLE)
        vkDestroyImageView(ld->device, sc->depthImage.common->imageView, NULL);
    if (sc->gBuffer && sc->gBuffer->imageView != VK_NULL_HANDLE)
        vkDestroyImageView(ld->device, sc->gBuffer->imageView, NULL);
    if (sc->nBuffer && sc->nBuffer->imageView != VK_NULL_HANDLE)
        vkDestroyImageView(ld->device, sc->nBuffer->imageView, NULL);

    // Enqueue VMA-managed swapchain images (MSAA, depth, gbuffer, normals)
    if (sc->msaaColorRes)
        deferDestroyImage(dq, sc->msaaColorRes->image, sc->msaaColorRes->allocation);
    if (sc->msaaDepthRes)
        deferDestroyImage(dq, sc->msaaDepthRes->image, sc->msaaDepthRes->allocation);
    if (sc->gBuffer)
    {
        deferDestroyImage(dq, sc->gBuffer->image, sc->gBuffer->allocation);
        free(sc->gBuffer);
        sc->gBuffer = NULL;
    }
    if (sc->nBuffer)
    {
        deferDestroyImage(dq, sc->nBuffer->image, sc->nBuffer->allocation);
        free(sc->nBuffer);
        sc->nBuffer = NULL;
    }
    if (sc->depthImage.common)
    {
        deferDestroyImage(dq, sc->depthImage.common->image, sc->depthImage.common->allocation);
        free(sc->depthImage.common);
        sc->depthImage.common = NULL;
    }

    vkDestroySwapchainKHR(ld->device, sc->VkSwapchain, VK_NULL_HANDLE);
}

void recreateSwapChain(PhysicalDevice* pd, LogicalDevice* ld, SDL_Window* window, RenderGraph* rg)
{
    vkDeviceWaitIdle(ld->device);
    cleanupSwapChain(ld, ld->displayManager);
    createSwapChain(pd, ld, ld->displayManager, ld->displayManager->graphicPipeline, window);

    GraphicSwapchain* sc = ld->displayManager->graphicPipeline->swapchain;

    createMsaaColorResources(ld, pd, sc);
    createMsaaDepthResources(ld, pd, sc);
    createDepthImageResources(pd, ld, sc);
    createGeometryBufferResources(pd, ld, sc);
    createNormalsBufferResources(pd, ld, sc);
    createGBufferSampler(ld->device, sc);

    for (uint32_t i = 0; i < sc->scImagesLen; i++)
    {
        sc->p_scImageView[i] = createImageView(
            sc, ld->device, sc->currentSurfaceFormat->format,
            sc->p_scImages[i], 1, VK_IMAGE_ASPECT_COLOR_BIT);
    }

    // Re-write G-buffer/normals descriptors (bindings 5/6) with new image views
    GraphicPipelineToSurface* gdp = ld->displayManager->graphicPipeline;
    updateGBufferDescriptors(gdp->maxFrames, ld->device, gdp);

    // Update render graph external resource handles to point at the new images
    if (rg)
    {
        rg->resources[0].image     = sc->p_scImages[0];
        rg->resources[0].imageView = sc->p_scImageView[0];

        rg->resources[1].image     = sc->depthImage.common->image;
        rg->resources[1].imageView = sc->depthImage.common->imageView;

        rg->resources[2].image     = sc->gBuffer->image;
        rg->resources[2].imageView = sc->gBuffer->imageView;

        rg->resources[3].image     = sc->nBuffer->image;
        rg->resources[3].imageView = sc->nBuffer->imageView;

        rgOnSwapchainResize(rg, sc->currentExtent2D.width, sc->currentExtent2D.height);
    }
}
