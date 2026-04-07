#include <iostream>
#include <vector>
#include "metaEngine.hpp"
#include "model.hpp"
#include <SDL_ttf.h>
#include <thread>
#include <random>
extern "C"
{

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
#include "strictDebug.h"
#include "main_vulkan.h"
#include "cglm.h"
#include "graphicsPipeline.h"
#include "graphicConfiguration.h"
#include "commandPool.h"
#include "displayManager.h"
#include "swapchainManager.h"
#include "uniformBuffer.h"
#include "descriptor.h"
#include "textureManager.h"
#include "scene.h"
#include "model.h"
};
#include <chrono>
// TODO move this to central display
const int MARGIN = 20;
const double TARGET_FPS = 100; // Assuming 60Hz refresh rate
const double FRAME_TIME = 1.0 / TARGET_FPS;

// Global variables for camera control
static int mouseLookActive = 0;
static float mouseSensitivity = 0.002f;
static float moveSpeed = 5.0f;
static int keys[256] = {0}; // Key state array

double getCurrentTime()
{
    auto now = std::chrono::high_resolution_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::duration<double>>(duration).count();
}

// Camera movement update (call this in your main loop with deltaTime)
void updateCamera(Camera3D *camera, float deltaTime)
{
    // Calculate movement vectors
    vec3 forward, right;
    glm_vec3_sub(camera->center, camera->eye, forward);
    glm_vec3_normalize(forward);
    glm_vec3_cross(forward, camera->up, right);
    glm_vec3_normalize(right);

    vec3 movement = {0.0f, 0.0f, 0.0f};
    float speed = moveSpeed * deltaTime;

    // WASD movement
    if (keys[SDL_SCANCODE_W])
    {
        // Move forward
        glm_vec3_muladds(forward, speed, movement);
    }
    if (keys[SDL_SCANCODE_S])
    {
        // Move backward
        glm_vec3_muladds(forward, -speed, movement);
    }
    if (keys[SDL_SCANCODE_A])
    {
        // Move left
        glm_vec3_muladds(right, -speed, movement);
    }
    if (keys[SDL_SCANCODE_D])
    {
        // Move right
        glm_vec3_muladds(right, speed, movement);
    }

    // Apply movement
    glm_vec3_add(camera->eye, movement, camera->eye);
    glm_vec3_add(camera->center, movement, camera->center);
}

float getRandomDouble(double max)
{
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<double> dis(0.0, max);
    return (float)dis(gen);
}
uint32_t min(uint32_t a, uint32_t b)
{
    return a < b ? a : b;
}
uint32_t max(uint32_t a, uint32_t b)
{
    return a > b ? a : b;
}
int main()
{

#ifdef NDEBUG
    std::cout << "Assertions are disabled" << std::endl;
#else
    std::cout << "Assertions arerrr enabled" << std::endl;
#endif

    try
    {
        // Before SDL_Init
        SDL_SetHint(SDL_HINT_VIDEO_WAYLAND_ALLOW_LIBDECOR, "x11");
        // Add these checks
        if (SDL_Init(SDL_INIT_VIDEO) < 0)
        {
            std::cerr << "SDL initialization failed: " << SDL_GetError() << std::endl;
            return -1;
        }
        std::cout << "Current video driver: " << SDL_GetCurrentVideoDriver() << std::endl;

        SDL_Window *window = NULL;
        window = SDL_CreateWindow("Hello World!", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_VULKAN);
        MetaEngine *engine = new MetaEngine();
        engine->initVkInstance(window);
        //        std::this_thread::sleep_for(std::chrono::seconds(5));
        if (window == NULL)
        {
            const char *error = SDL_GetError();
            std::cout << error << std::endl;
            SDL_Quit();
            return -1;
        }
        auto scene = std::make_unique<Scene>(Scene());
        const float cubeSize = 3.0f;
        const float spacing = 3.0f;
        const float totalSize = cubeSize + spacing;
        LightVolume *light = static_cast<LightVolume *>(malloc(sizeof(LightVolume)));
        GEOM_VERTEX_POSITION *cubeLightPosition = new GEOM_VERTEX_POSITION{0.0, 0.0, 0.0};
        light->model = *create_cube_no_vertex_optimization(cubeLightPosition, 10);
        printLightVolume(light);
        scene->addLight(light);
        ParticleGroup *particleGroup = (ParticleGroup *)malloc(sizeof(ParticleGroup));

        scene->initParticle();
        uint32_t particleInstancesLen = 1000000;
        ParticleInstance *pParticleInstance = (ParticleInstance *)malloc(sizeof(ParticleInstance) * particleInstancesLen);
        for (uint32_t partInstanceIndex = 0; partInstanceIndex < particleInstancesLen; partInstanceIndex++)
        {
            float x = 0.0;
            float y = 0.0;
            float z = 0.0;
            vec4 tempPos = {(float)partInstanceIndex, (float)partInstanceIndex, (float)partInstanceIndex, 0.0f};
            glm_vec4_copy(tempPos, pParticleInstance[partInstanceIndex].position);

            vec4 tempVel = {
                1.0,                // x
                getRandomDouble(0), // y
                getRandomDouble(0), // z
                0.0f                // w = 0.0
            };
            glm_vec4_copy(tempVel, pParticleInstance[partInstanceIndex].velocity);

            vec4 tempColor = {
                getRandomDouble(100), // r
                getRandomDouble(100), // g
                getRandomDouble(100), // b
                0.0f                  // a = 0.0
            };
            glm_vec4_copy(tempColor, pParticleInstance[partInstanceIndex].color);
        }

        scene->initInstanceDataParticle(particleGroup, particleInstancesLen, pParticleInstance);
        scene->addParticleGroup(particleGroup);
        vec3 eye = {1.5f, 1.5f, 1.5f};
        vec3 center = {0.0f, 0.0f, 0.0f};
        vec3 up = {0.0f, 0.0f, 1.0f};
        vec2 screenSize = {WINDOW_WIDTH, WINDOW_HEIGHT};
        glm_vec2_print(screenSize, stdout);
        Camera3D *camera = (Camera3D *)malloc(sizeof(*camera));
        glm_vec3_copy(eye, camera->eye);
        glm_vec3_copy(center, camera->center);
        glm_vec3_copy(up, camera->up);
        glm_vec2_copy(screenSize, camera->screenSize);
        scene->addCamera3D(camera);
        GEOM_VERTEX_COLOR_RGBA lightColors[2000];
        vec3                   lightPositions[2000];
        float                  lightScales[2000];

        vec3 vecPos  = {cubeLightPosition->x + 5, cubeLightPosition->y, cubeLightPosition->z};
        vec3 vecPos2 = {cubeLightPosition->x, cubeLightPosition->y, cubeLightPosition->z};

        for (int i = 0; i < 2000; i++)
        {
            if (i % 2 == 0)
            {
                lightColors[i] = {0.0, 1.0, 0.0, 1.0};
                glm_vec3_copy(vecPos, lightPositions[i]);
                lightScales[i] = 1.0f;
            }
            else
            {
                lightColors[i] = {1.0, 0.0, 0.0, 1.0};
                glm_vec3_copy(vecPos2, lightPositions[i]);
                lightScales[i] = 1.0f;
            }
        }
        scene->initInstanceDataLight(&scene->getVolumetricLight(0), 2000,
                                     lightColors, lightPositions, lightScales);
        // GEOM_VERTEX_POSITION* planeCenter = new GEOM_VERTEX_POSITION{0, 0, 0};
        // scene->addModel(std::move(Model<GEOM_OBJECT>(PRIMITIVE_TYPE::PLANE, planeCenter, 1000)));

        scene->addModel(load_obj_to_geom_object(PREPEND_ABSOLUTE_PATH("assets/models/viking_room.obj")));

        // scene->addModel(&light->model);
        scene->initInstanceDataModel(&scene->getModel(0), 2000);

        scene->init();
        std::cout << "QUE??????\n";
        engine->bulkInitVulkanWithScene(window, scene.get());
        // Add these variables at the beginning of your main function or in your game class
        TTF_Font *font = nullptr;
        SDL_Color textColor = {255, 255, 255, 255}; // White color
        Uint32 frameStart, frameTime;
        int fps = 0;
        int frameCount = 0;
        float avgFPS = 0.0f;
        std::cout << "ESTO QUE MIERDA ES" << std::endl;
        uint32_t running = 1;
        bool resizing = 0;
        bool needsSwapchainRecreate = false;
        int pendingResizeW = 0, pendingResizeH = 0;
        SDL_Event event;
        int initialMouseX, initialMouseY;
        int initialWidth, initialHeight;
        double lastFrameTime = getCurrentTime();
        while (running)
        {
            double currentTime = getCurrentTime();
            double deltaTime = currentTime - lastFrameTime;
            SDL_Event event;
            while (SDL_PollEvent(&event))
            {

                switch (event.type)
                {
                case SDL_MOUSEBUTTONDOWN:
                    switch (event.button.button)
                    {
                    case SDL_BUTTON_LEFT:
                    {
                        int mouseX = event.button.x;
                        int mouseY = event.button.y;
                        int winWidth, winHeight;
                        SDL_GetWindowSize(window, &winWidth, &winHeight);

                        if (mouseX >= winWidth - MARGIN && mouseY >= winHeight - MARGIN)
                        {
                            resizing = 1;
                            initialMouseX = mouseX;
                            initialMouseY = mouseY;
                            initialWidth = winWidth;
                            initialHeight = winHeight;
                        }
                    }
                    break;

                    case SDL_BUTTON_RIGHT:
                        // Enable mouse look
                        mouseLookActive = 1;
                        SDL_SetRelativeMouseMode(SDL_TRUE);
                        break;
                    }
                    break;

                case SDL_MOUSEBUTTONUP:
                    switch (event.button.button)
                    {
                    case SDL_BUTTON_LEFT:
                        if (resizing)
                            needsSwapchainRecreate = true;
                        resizing = 0;
                        break;
                    case SDL_BUTTON_RIGHT:
                        // Disable mouse look
                        mouseLookActive = 0;
                        SDL_SetRelativeMouseMode(SDL_FALSE);
                        break;
                    }
                    break;

                case SDL_MOUSEMOTION:
                    if (resizing)
                    {
                        pendingResizeW = initialWidth + (event.motion.x - initialMouseX);
                        pendingResizeH = initialHeight + (event.motion.y - initialMouseY);
                        if (pendingResizeW < 128)
                            pendingResizeW = 128;
                        if (pendingResizeH < 128)
                            pendingResizeH = 128;
                    }
                    else if (mouseLookActive)
                    {
                        float deltaX = event.motion.xrel * mouseSensitivity;
                        float deltaY = event.motion.yrel * mouseSensitivity;

                        // Calculate current forward direction
                        vec3 forward;
                        glm_vec3_sub(camera->center, camera->eye, forward);
                        float distance = glm_vec3_norm(forward);
                        glm_vec3_normalize(forward);

                        // Calculate right vector
                        vec3 right;
                        glm_vec3_cross(forward, camera->up, right);
                        glm_vec3_normalize(right);

                        // Calculate true up vector
                        vec3 trueUp;
                        glm_vec3_cross(right, forward, trueUp);

                        // Apply horizontal rotation (yaw)
                        vec3 rotatedForward;
                        glm_vec3_rotate(forward, deltaX, camera->up);

                        // Apply vertical rotation (pitch)
                        glm_vec3_rotate(forward, deltaY, right);

                        // Update center position
                        glm_vec3_scale(forward, distance, forward);
                        glm_vec3_add(camera->eye, forward, camera->center);
                    }
                    break;

                case SDL_KEYDOWN:
                    if (event.key.keysym.scancode < 256)
                    {
                        keys[event.key.keysym.scancode] = 1;
                    }
                    // E key: enable model 0 (all instances alive)
                    if (event.key.keysym.scancode == SDL_SCANCODE_E)
                    {
                        enableModel(&scene->getScene()->modelsManager, 0);
                        std::cout << "[demo] Model 0 enabled" << std::endl;
                    }
                    // R key: disable model 0 (all instances skipped)
                    if (event.key.keysym.scancode == SDL_SCANCODE_R)
                    {
                        disableModel(&scene->getScene()->modelsManager, 0);
                        std::cout << "[demo] Model 0 disabled" << std::endl;
                    }
                    // I key: toggle instance 0 of model 0
                    if (event.key.keysym.scancode == SDL_SCANCODE_I)
                    {
                        Models3DManager *man = &scene->getScene()->modelsManager;
                        int state = queryInstanceAlive(man, 0, 0);
                        if (state == INSTANCE_ALIVE)
                        {
                            disableInstance(man, 0, 0);
                            std::cout << "[demo] Instance 0 of model 0 disabled" << std::endl;
                        }
                        else
                        {
                            enableInstance(man, 0, 0);
                            std::cout << "[demo] Instance 0 of model 0 re-enabled" << std::endl;
                        }
                    }
                    // T key: toggle model 0 visibility
                    if (event.key.keysym.scancode == SDL_SCANCODE_T)
                    {
                        Models3DManager *man = &scene->getScene()->modelsManager;
                        if (queryModelAlive(man, 0) == MODEL_ALIVE)
                        {
                            disableModel(man, 0);
                            std::cout << "[demo] Model 0 disabled" << std::endl;
                        }
                        else
                        {
                            enableModel(man, 0);
                            std::cout << "[demo] Model 0 re-enabled" << std::endl;
                        }
                    }
                    // J key: permanently destroy instance 0 of model 0
                    if (event.key.keysym.scancode == SDL_SCANCODE_J)
                    {
                        destroyInstance(&scene->getScene()->modelsManager, 0, 0);
                        std::cout << "[demo] Instance 0 of model 0 permanently destroyed" << std::endl;
                    }
                    // K key: permanently destroy model 0
                    if (event.key.keysym.scancode == SDL_SCANCODE_K)
                    {
                        destroyModel(&scene->getScene()->modelsManager, 0);
                        std::cout << "[demo] Model 0 permanently destroyed" << std::endl;
                    }
                    // L key: toggle light 0 visibility
                    if (event.key.keysym.scancode == SDL_SCANCODE_L)
                    {
                        LightManager *lm = &scene->getScene()->lightManager;
                        if (queryLightAlive(lm, 0) == LIGHT_ALIVE)
                        {
                            disableLight(lm, 0);
                            std::cout << "[demo] Light 0 disabled" << std::endl;
                        }
                        else
                        {
                            enableLight(lm, 0);
                            std::cout << "[demo] Light 0 re-enabled" << std::endl;
                        }
                    }
                    // O key: toggle light instance 0 of light 0
                    if (event.key.keysym.scancode == SDL_SCANCODE_O)
                    {
                        LightManager *lm = &scene->getScene()->lightManager;
                        int state = queryLightInstanceAlive(lm, 0, 0);
                        if (state == LIGHT_INSTANCE_ALIVE)
                        {
                            disableLightInstance(lm, 0, 0);
                            std::cout << "[demo] Light instance 0 of light 0 disabled" << std::endl;
                        }
                        else
                        {
                            enableLightInstance(lm, 0, 0);
                            std::cout << "[demo] Light instance 0 of light 0 re-enabled" << std::endl;
                        }
                    }
                    // P key: permanently destroy light instance 0 of light 0
                    if (event.key.keysym.scancode == SDL_SCANCODE_P)
                    {
                        destroyLightInstance(&scene->getScene()->lightManager, 0, 0);
                        std::cout << "[demo] Light instance 0 of light 0 permanently destroyed" << std::endl;
                    }
                    break;

                case SDL_KEYUP:
                    if (event.key.keysym.scancode < 256)
                    {
                        keys[event.key.keysym.scancode] = 0;
                    }
                    break;
                case SDL_QUIT:
                    running = 0;
                    break;
                }
            }

            // Apply accumulated resize once per frame (not per mouse event)
            if (resizing && pendingResizeW > 0)
            {
                SDL_SetWindowSize(window, pendingResizeW, pendingResizeH);
                pendingResizeW = 0;
                pendingResizeH = 0;
            }

            // Deferred swapchain recreation after resize drag ends
            if (needsSwapchainRecreate)
            {
                engine->metaRecreateSwapchain(window);
                needsSwapchainRecreate = false;

                // Update camera screen size to match the new swapchain extent
                int w, h;
                SDL_Vulkan_GetDrawableSize(window, &w, &h);
                camera->screenSize[0] = (float)w;
                camera->screenSize[1] = (float)h;
            }

            // Llamada a la función drawFrame
            if (deltaTime >= FRAME_TIME)
            {
                updateCamera(scene->getCamera3D(), deltaTime);
                // Render and present frame
                if (!resizing)
                    engine->metaDrawFrame(window, (float)deltaTime);

                lastFrameTime = currentTime;

                // Frame pacing: Sleep to align with next vsync
                double sleepTime = FRAME_TIME - (getCurrentTime() - currentTime);
                if (sleepTime > 0)
                {
                    sleep(sleepTime);
                }
            }

            // Aquí puedes añadir tus comandos de dibujo
            //        SDL_RenderPresent(renderer);
        }

        engine->metaCleanup();
#ifndef RENDERDOC_ENABLED
        delete engine;
#endif
        SDL_DestroyWindow(window);
        SDL_Quit();
    }
    catch (const std::exception &e)
    {
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
