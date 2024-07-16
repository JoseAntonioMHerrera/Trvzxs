#pragma once
#include <SDL2/SDL.h>
#include <stddef.h>
#include "deviceManagmentTypes.h"
#include "uniformBuffer.h"
#include "swapchainManager.h"
#include "graphicsPipeline.h"
#define MAX_FRAMES_IN_FLIGHT 2

void  drawFrame(PhysicalDevice *pd, LogicalDevice *ld, DisplayManager *dm, GeometryDisplayPipelineManager *gdp,
VkDevice device, SDL_Window *window);

TRVZXS_inline void cleanupSyncObjects(VkDevice device, GeometryDisplayPipelineManager* gdp){

    for (size_t i = 0; i < gdp->maxFrames; i++) {
        vkDestroySemaphore(device, gdp->renderEndFlag[i], NULL);
        vkDestroySemaphore(device, gdp->imgViewSemaphore[i], NULL);
        vkDestroyFence(device, gdp->inFlightFence[i], NULL);
    }

}

TRVZXS_inline void createSDLSurface(DisplayManager *dm, VkInstance* instance, SDL_Window* window){
    SDL_bool result_surface_creation = SDL_Vulkan_CreateSurface(window,
                                  *instance,
                                  &dm->surface);
    printf("CREATE INSTANCE FLAG 5\n");
    assert(dm->surface);
    assert(*instance);

    assert(result_surface_creation == SDL_TRUE);
}
