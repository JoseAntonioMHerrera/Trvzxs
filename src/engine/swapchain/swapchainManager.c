#include "swapchainManager.h"
#include "image.h"
#include "deletionQueue.h"
#include "descriptor.h"
#include "renderGraph.h"
#include "graphicConfiguration.h"
#include "graphicsPipeline.h"
#include "displayManager.h"
#include "bindless.h"
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
void chooseSwapExtent2D(VkExtent2D* extent, VkSurfaceCapabilitiesKHR* capabilities, SDL_Window* window)
{
    int width, height;
    SDL_Vulkan_GetDrawableSize(window, &width, &height);
#ifdef DEBUG
    fprintf(stderr, "[swapchain] SDL drawable size: %d x %d\n", width, height);
#endif

    // Enforce the configured resolution so profiling numbers are reproducible
    width  = WINDOW_WIDTH;
    height = WINDOW_HEIGHT;

    extent->width  = CLAMP(width, capabilities->minImageExtent.width, capabilities->maxImageExtent.width);
    extent->height = CLAMP(height, capabilities->minImageExtent.height, capabilities->maxImageExtent.height);
#ifdef DEBUG
    fprintf(stderr, "[swapchain] Enforced extent: %u x %u\n", extent->width, extent->height);
#endif
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

#ifdef DEBUG
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
#endif

VkPresentModeKHR chooseSwapPresentMode(VkPresentModeKHR* availablePresentModes, uint8_t availableModesCount,
                                       VkPresentModeKHR mode)
{
#ifdef DEBUG
    printf("[swapchain] requested present mode: %s\n", presentModeName(mode));
    printf("[swapchain] surface advertises %u present mode(s):\n", availableModesCount);
    for (uint8_t i = 0; i < availableModesCount; i++)
    {
        printf("              [%u] %s\n", i, presentModeName(availablePresentModes[i]));
    }
#endif
    for (uint8_t i = 0; i < availableModesCount; i++)
    {
        if (availablePresentModes[i] == mode)
        {
#ifdef DEBUG
            printf("[swapchain] GRANTED: %s\n", presentModeName(availablePresentModes[i]));
            fflush(stdout);
#endif
            return availablePresentModes[i];
        }
    }
#ifdef DEBUG
    printf("[swapchain] FALLBACK to %s — requested mode not supported by surface\n",
           presentModeName(VK_PRESENT_MODE_FIFO_KHR));
    fflush(stdout);
#endif
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

    // Clamp to the capacity of p_scImages / p_scImageView.
    if (gps->swapchain->scImagesLen > SC_MAX_IMAGES)
        gps->swapchain->scImagesLen = SC_MAX_IMAGES;

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

    // The driver may create more images than requested; every one of them
    // can be returned by acquire, so they must all fit.
    if (gps->swapchain->scImagesLen > SC_MAX_IMAGES)
    {
        fprintf(stderr, "[swapchain] driver created %u images, capacity is %d (SC_MAX_IMAGES)\n",
                gps->swapchain->scImagesLen, SC_MAX_IMAGES);
        exit(1);
    }

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
    destroySwapchainSemaphores(&ld->frameSync, ld->device);
    createSwapChain(pd, ld, ld->displayManager, ld->displayManager->graphicPipeline, window);

    GraphicSwapchain* sc = ld->displayManager->graphicPipeline->swapchain;

    createMsaaColorResources(ld, pd, sc);
    createMsaaDepthResources(ld, pd, sc);
    createDepthImageResources(pd, ld, sc);
    createGeometryBufferResources(pd, ld, sc);
    createNormalsBufferResources(pd, ld, sc);
    createGBufferSampler(ld->device, sc);
    createSwapchainSemaphores(&ld->frameSync, ld->device, sc->scImagesLen);

    for (uint32_t i = 0; i < sc->scImagesLen; i++)
    {
        sc->p_scImageView[i] = createImageView(
            sc, ld->device, sc->currentSurfaceFormat->format,
            sc->p_scImages[i], 1, VK_IMAGE_ASPECT_COLOR_BIT);
    }

    // Re-write G-buffer/normals descriptors (bindings 5/6) with new image views
    GraphicPipelineToSurface* gdp = ld->displayManager->graphicPipeline;
    // updateGBufferDescriptors(ld->frameSync.maxFramesInFlight, ld->device, gdp); // DORMANT A11 — light pass uses bindless per-pass set

    // A11 — bindless light per-pass set holds the G-buffer + normals samplers
    // at bindings 1,2. After swapchain recreate the image views are fresh;
    // rewrite both bindings across all in-flight frame sets.
    GraphicPipelineToSurface* lightGdp = &ld->displayManager->graphicPipeline[2];
    bindlessWriteLightPassGBufferSamplers(ld->device, ld->frameSync.maxFramesInFlight,
                                          lightGdp->perPassSets,
                                          sc->gBuffer->imageView,
                                          sc->nBuffer->imageView,
                                          sc->gBufferSampler);

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


        rgOnSwapchainResize(rg, sc->currentExtent2D.width, sc->currentExtent2D.height, sc->scImagesLen);
    }
}
