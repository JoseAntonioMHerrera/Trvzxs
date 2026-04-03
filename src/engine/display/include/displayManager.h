#pragma once
#include <SDL2/SDL.h>
#include <stddef.h>
#include "deviceManagmentTypes.h"
#include "uniformBuffer.h"
#include "swapchainManager.h"
#include "graphicsPipeline.h"
#define MAX_FRAMES_IN_FLIGHT 2

typedef struct RenderGraph RenderGraph;
void  drawFrame(PhysicalDevice *pd, LogicalDevice *ld, DisplayManager *dm, GraphicPipelineToSurface *gdp,
GraphicPipelineToSurface *particleGraphics, GraphicPipelineToSurface* lightGraphics, VkDevice device, SDL_Window *window, float deltaTime, Camera3D camera, RenderGraph* rg);
void cleanupSyncObjects(VkDevice device, GraphicPipelineToSurface* gdp);

void createSDLSurface(DisplayManager *dm, VkInstance* instance, SDL_Window* window);
