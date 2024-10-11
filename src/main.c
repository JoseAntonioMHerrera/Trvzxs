#include <SDL.h>
#include <stdio.h>
#include <stdbool.h>
#include <stddef.h>
#include <assert.h>
#include <X11/Xlib.h>
#include <unistd.h>
#include "main_vulkan.h"
#include "file_functions.h"
#include "displayManager.h"


/*
// Tamaño de la ventana
const int WINDOW_WIDTH = 640;
const int WINDOW_HEIGHT = 480;
const int MARGIN = 20;
//int main(int argc, char* argv[]) {
int engineEntryPoint(){
    // Inicializar SDL
    SDL_Window* window = NULL;
    window = SDL_CreateWindow("Hello World!", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_VULKAN);
    assert(window != NULL);
    /*if (SDL_Init(SDL_INIT_VIDEO) < 0) {

        return -1;
    }

    if (window == NULL) {

        SDL_Quit();
        return -1;
    }


    DeviceManager *dm = initTRVZXSEngine(window);

    DisplayManager *dispM = dm->lDevice[dm->deviceIndex]->displayManager;
    LogicalDevice *ld = dm->lDevice[dm->deviceIndex];
    PhysicalDevice *pd = &dm->pDevice[dm->gpuIndex];
    GraphicPipelineToSurface *gdp = dispM->graphicPipeline;
    GraphicSwapchain *dsm = dispM->swapchain;

   // Crear renderizador para la ventana
    /*SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == NULL) {

        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }
    uint32_t running = 1;
    bool resizing = 0;
    SDL_Event event;
    int initialMouseX, initialMouseY;
    int initialWidth, initialHeight;
    // Esperar 3 segundos
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {

            switch (event.type) {
                case SDL_QUIT:












                    running = 0;
                    break;
                case SDL_MOUSEBUTTONDOWN:
                    if (event.button.button == SDL_BUTTON_LEFT) {
                        int mouseX = event.button.x;
                        int mouseY = event.button.y;
                        int winWidth, winHeight;
                        SDL_GetWindowSize(window, &winWidth, &winHeight);

                        if (mouseX >= winWidth - MARGIN && mouseY >= winHeight - MARGIN) {
                            resizing = 1;
                            initialMouseX = mouseX;
                            initialMouseY = mouseY;
                            initialWidth = winWidth;
                            initialHeight = winHeight;
                        }
                    }
                    break;
                case SDL_MOUSEBUTTONUP:
                    if (event.button.button == SDL_BUTTON_LEFT) {
                        resizing = 0;
                    }
                    break;
                case SDL_MOUSEMOTION:
                    if (resizing) {
                        int mouseX = event.motion.x;
                        int mouseY = event.motion.y;
                        int newWidth = initialWidth + (mouseX - initialMouseX);
                        int newHeight = initialHeight + (mouseY - initialMouseY);
                        if (newWidth > 100 && newHeight > 100) {
                            SDL_SetWindowSize(window, newWidth, newHeight);
                        }
                    }
                    break;
            }
        }

        // Llamada a la función drawFrame
        if(!resizing) {

            drawFrame(pd, ld, dispM, gdp, ld->device, window);

        }



        // Aquí puedes añadir tus comandos de dibujo
//        SDL_RenderPresent(renderer);
    }
    // Liberar recursos




    cleanup(dm, ld, dispM, gdp,dm->logCallback);
  //  SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}*/
