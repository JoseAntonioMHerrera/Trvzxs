#include <iostream>
#include <vector>
#include "metaEngine.hpp"
#include "model.hpp"

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

int main() {

    #ifdef NDEBUG
    std::cout << "Assertions are disabled" << std::endl;
    #else
    std::cout << "Assertions are enabled" << std::endl;
    #endif

    try {
        auto global = GEOM_VERTEX{5,5,0};
        Scene<SCENE, Model, GEOM_OBJECT> scene;
        scene.addModel(std::move(Model<GEOM_OBJECT>(PRIMITIVE_TYPE::CUBE, &global, 3)));
        scene.init();

        SDL_Window* window = NULL;
        window = SDL_CreateWindow("Hello World!", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 800, SDL_WINDOW_VULKAN);
        assert(window != NULL);
        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
            printf("SDL no pudo inicializarse! SDL Error: %s\n", SDL_GetError());
            return -1;
        }

        if (window == NULL) {
            printf("La ventana no pudo crearse! SDL Error: %s\n", SDL_GetError());
            SDL_Quit();
            return -1;
        }

        std::cout <<" nos trajo para que el se vaya" << std::endl;
        auto engine =  new MetaEngine<SCENE, Model, GEOM_OBJECT>(window, scene);
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
