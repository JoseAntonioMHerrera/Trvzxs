#include <iostream>
#include <vector>
#include "model.hpp"

extern "C" {
    #include <SDL_stdinc.h>
    #include <SDL_vulkan.h>
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
    #include "primitives.h"
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
        std::vector<Model<GEOM_OBJECT>> scene;
        scene.emplace_back(std::move(Model<GEOM_OBJECT>(PRIMITIVE_TYPE::CUBE, &global, 3)));
        std::cout << "HOLA HOLA CARACOLA" << std::endl;
        std::cout << scene.back() << std::endl;
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
