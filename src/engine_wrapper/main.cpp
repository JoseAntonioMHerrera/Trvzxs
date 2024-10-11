#include <iostream>
#include <vector>
#include "metaEngine.hpp"
#include "model.hpp"
#include <SDL_ttf.h>
extern "C" {

    #include <SDL_stdinc.h>
    #include <SDL_vulkan.h>
    #include <SDL.h>
    #include <stdint.h>
    #include <time.h>
    #include <vulkan/vulkan.h>
    #include <assert.h>
    #include "string_helper_macros.h"
    #include "comparation_helper_macros.h"
    #include <unistd.h>
    #include <vulkan/vulkan_core.h>
    #include "file_functions.h"
    #include "vertex.h"
    #include <vulkan/vulkan_core.h>
    #include "strictDebug.h"
    #include "main_vulkan.h"
    #include "cglm.h"
    #include "graphicsPipeline.h"
    #include "commandPool.h"
    #include "displayManager.h"
    #include "swapchainManager.h"
    #include "uniformBuffer.h"
    #include "descriptor.h"
    #include "textureManager.h"
    #include "scene.h"
};
#include <chrono>

const int WINDOW_WIDTH = 640;
const int WINDOW_HEIGHT = 480;
const int MARGIN = 20;
const double TARGET_FPS = 200.0;  // Assuming 60Hz refresh rate
const double FRAME_TIME = 1.0 / TARGET_FPS;


double getCurrentTime() {
    auto now = std::chrono::high_resolution_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::duration<double>>(duration).count();
}



int main() {

    #ifdef NDEBUG
    std::cout << "Assertions are disabled" << std::endl;
    #else
    std::cout << "Assertions are enabled" << std::endl;
    #endif

    try {



        SDL_Window* window = NULL;
        window = SDL_CreateWindow("Hello World!", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_VULKAN);
        assert(window != NULL);
        if (SDL_Init(SDL_INIT_VIDEO) < 0) {

            return -1;
        }

        if (window == NULL) {

            SDL_Quit();
            return -1;
        }
        std::unique_ptr<Scene<SCENE, Model, GEOM_OBJECT>> scene = std::make_unique<Scene<SCENE, Model, GEOM_OBJECT>>(Scene<SCENE, Model, GEOM_OBJECT>(sceneDeletor));
      const float cubeSize = 3.0f;
        const float spacing = 3.0f;
        const float totalSize = cubeSize + spacing;
        /*const int dimensions = 1000;
        const int numCubesPerDimension = static_cast<int>(dimensions / totalSize);

        for (int i = 0; i < numCubesPerDimension; ++i) {
            for (int j = 0; j < numCubesPerDimension; ++j) {
                for (int k = 0; k < numCubesPerDimension; ++k) {
                    float x = i * totalSize;
                    float y = j * totalSize;
                    float z = k * totalSize;
                    //std::cout << "VAMOS A AÑADIR EL VERTICE (" << x <<", "<< y << ", "<< z << '\n';
                    GEOM_VERTEX_POSITION* cubeOrigin = new GEOM_VERTEX_POSITION{x, y, z};
                    scene->addModel(std::move(Model<GEOM_OBJECT>(PRIMITIVE_TYPE::CUBE, cubeOrigin, cubeSize)));
                }
            }
        }*/

        //scene->addModel(std::move(Model<GEOM_OBJECT>(PRIMITIVE_TYPE::CUBE, &global, 3)));
        //GEOM_VERTEX_POSITION* cubeOrigin = new GEOM_VERTEX_POSITION{0, 0, 0};
        //scene->addModel(std::move(Model<GEOM_OBJECT>(PRIMITIVE_TYPE::QUAD, cubeOrigin, cubeSize)));

        scene->addModel(std::move(Model<GEOM_OBJECT>(PREPEND_ABSOLUTE_PATH("assets/models/viking_room.obj"))));
        scene->init();
        MetaEngine<SCENE, Model, GEOM_OBJECT> *engine =  new MetaEngine<SCENE, Model, GEOM_OBJECT>(window, std::move(scene));

        // Add these variables at the beginning of your main function or in your game class
TTF_Font* font = nullptr;
SDL_Color textColor = {255, 255, 255, 255}; // White color
Uint32 frameStart, frameTime;
int fps = 0;
int frameCount = 0;
float avgFPS = 0.0f;

    uint32_t running = 1;
    bool resizing = 0;
    SDL_Event event;
    int initialMouseX, initialMouseY;
    int initialWidth, initialHeight;
    double lastFrameTime = getCurrentTime();
        while (running) {
            double currentTime = getCurrentTime();
            double deltaTime = currentTime - lastFrameTime;
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
             if (deltaTime >= FRAME_TIME) {
        // Render and present frame
                    engine->metaDrawFrame(window);

        lastFrameTime = currentTime;

        // Frame pacing: Sleep to align with next vsync
        double sleepTime = FRAME_TIME - (getCurrentTime() - currentTime);
        if (sleepTime > 0) {
            sleep(sleepTime);
        }
    }



        }



        // Aquí puedes añadir tus comandos de dibujo
//        SDL_RenderPresent(renderer);
    }

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
/*
   try {
        engineEntryPoint();
    } catch (const std::exception& e) {
        std::cerr << "Caught exception: " << e.what() << std::endl;
        throw;  // Re-throw the exception
    } catch (...) {
        std::cerr << "Caught unknown exception" << std::endl;
        throw;  // Re-throw the exception
    }*/

    return 0;
}
