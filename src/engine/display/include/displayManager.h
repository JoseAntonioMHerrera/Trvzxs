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

void cleanupSyncObjects(VkDevice device, GeometryDisplayPipelineManager* gdp);

void createSDLSurface(DisplayManager *dm, VkInstance* instance, SDL_Window* window);
