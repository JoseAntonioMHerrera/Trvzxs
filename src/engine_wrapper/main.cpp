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
const int LIGHT_INSTANCE_COUNT = 2;
const int MODEL_INSTANCE_COUNT = 20000;

const double TARGET_FPS = 60; // Assuming 60Hz refresh rate
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

// ---------------------------------------------------------------------------
// Benchmark harness
// ---------------------------------------------------------------------------
//
// Runs MAX_FRAME_COUNT frames split across a sequence of fixed camera
// scenarios, then exits. Each scenario sets camera eye/center/up on entry
// and holds for its frameCount frames, so profile output from display.c
// (resolve/cull/UBO fill timings) is labelled by scenario in the run log.
//
// On top of the camera sequence, every frame dirties a fixed fraction of
// model instances (BENCH_DIRTY_FRACTION) via small random translations and
// transformMarkDirty. This gives the dirty-flag resolve skip a nontrivial
// load — neither "static scene" (best case) nor "everything moves" (worst
// case).
//
// RNG is a deterministic LCG seeded at startup so successive runs process
// the same instance set in the same order.

struct BenchmarkScenario
{
    const char *name;
    vec3 eye;
    vec3 center;
    vec3 up;
    uint32_t frameCount;
};

// 40 + 40 + 80 + 80 + 80 = 320 frames, matches MAX_FRAME_COUNT. Scenarios
// 3/4/5 get double weight because they're in the CPU-visible regime where the
// per-frame variance dominates; scenarios 1/2 are already fence-blocked on the
// GPU wall so 40 is plenty.
static const BenchmarkScenario g_benchmarkScenarios[] = {
    // 1. Top-down overview: camera high above the 10×10 grid (spans ±90 on X/Y).
    //    Most groups visible. Exercises the "many tests, few bulk skips" path.
    {"top-down overview", {0.0f, 0.0f, 260.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, 40},

    // 2. Low edge grazing: camera at Y=-180 looking toward the grid along +Y.
    //    About half the groups in view; distant ones get bulk-skipped.
    {"low edge grazing", {0.0f, -180.0f, 20.0f}, {0.0f, 20.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, 40},

    // 3. Close-up on one corner group: camera near (-90,-90) looking at that
    //    single group. 99% of the scene outside the frustum → ideal bulk-skip.
    {"corner close-up", {-95.0f, -95.0f, 15.0f}, {-85.0f, -85.0f, 5.0f}, {0.0f, 0.0f, 1.0f}, 80},

    // 4. Facing away from the grid: camera at origin looking -Z. Zero groups
    //    in frustum → 100 light roots tested, all culled, 20000 models
    //    bulk-skipped. The "cheapest cull" case.
    {"facing away (full cull)", {0.0f, 0.0f, 5.0f}, {0.0f, 0.0f, -95.0f}, {0.0f, 1.0f, 0.0f}, 80},

    // 5. Inside the grid looking along +X: a strip of groups visible across
    //    the middle of the frustum, others outside.
    {"inside grid, +X view", {0.0f, 0.0f, 10.0f}, {60.0f, 0.0f, 10.0f}, {0.0f, 0.0f, 1.0f}, 80},
};
static const uint32_t g_benchmarkScenarioCount =
    sizeof(g_benchmarkScenarios) / sizeof(g_benchmarkScenarios[0]);

// Fraction of model instances to dirty per frame.
static const float BENCH_DIRTY_FRACTION = 0.20f;

// Small LCG. Deterministic across runs for reproducible benchmarks.
static uint32_t benchRng_nextU32(uint32_t *state)
{
    *state = (*state) * 1664525u + 1013904223u;
    return *state;
}

static float benchRng_signedUnit(uint32_t *state)
{
    // Range approximately [-1, 1]
    return ((float)(benchRng_nextU32(state) & 0xFFFF) / 32768.0f) - 1.0f;
}

// Apply the scenario for the given frame index. Logs on scenario entry.
static void benchApplyCamera(uint32_t frameIdx, Camera3D *camera)
{
    uint32_t cumulative = 0;
    for (uint32_t s = 0; s < g_benchmarkScenarioCount; s++)
    {
        uint32_t next = cumulative + g_benchmarkScenarios[s].frameCount;
        if (frameIdx < next)
        {
            // Set camera every frame of the scenario so nothing drifts it.
            glm_vec3_copy(const_cast<float *>(g_benchmarkScenarios[s].eye), camera->eye);
            glm_vec3_copy(const_cast<float *>(g_benchmarkScenarios[s].center), camera->center);
            glm_vec3_copy(const_cast<float *>(g_benchmarkScenarios[s].up), camera->up);

            if (frameIdx == cumulative)
            {
                std::cout << "\n[bench] frame " << frameIdx << " → scenario '"
                          << g_benchmarkScenarios[s].name << "' (" << g_benchmarkScenarios[s].frameCount
                          << " frames)" << std::endl;
                // Flush previous scenario's stats and start a new sample window.
                displayBenchBeginScenario(g_benchmarkScenarios[s].name);
            }
            return;
        }
        cumulative = next;
    }
}

// Dirty BENCH_DIRTY_FRACTION of model instances by translating them a
// small random amount and marking them dirty. Exercises the dirty-flag
// skip under realistic partial-mutation load.
static void benchApplyRandomDirty(SCENE *scene)
{
    static uint32_t rngState = 0xDEADBEEFu;

    Models3DManager *mm = &scene->modelsManager;
    TransformManager *tman = &scene->transformManager;

    uint32_t total = mm->modelInstanceCapacity;
    if (total == 0)
        return;

    uint32_t toUpdate = (uint32_t)((float)total * BENCH_DIRTY_FRACTION);
    for (uint32_t k = 0; k < toUpdate; k++)
    {
        uint32_t idx = benchRng_nextU32(&rngState) % total;
        uint32_t tIdx = mm->pDenseModelInstances[idx].transformIdx;

        vec3 delta = {
            benchRng_signedUnit(&rngState) * 0.01f,
            benchRng_signedUnit(&rngState) * 0.01f,
            benchRng_signedUnit(&rngState) * 0.01f,
        };

        transformTranslate(&tman->pTransformsCold[tIdx], delta);
        transformMarkDirty(tman, tIdx);
    }
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
        vec3 eye = {0.0f, 0.0f, 0.0f};
        vec3 center = {5.0f, 0.0f, 0.0f};
        vec3 up = {0.0f, 0.0f, 1.0f};
        vec2 screenSize = {WINDOW_WIDTH, WINDOW_HEIGHT};
        glm_vec2_print(screenSize, stdout);
        Camera3D *camera = (Camera3D *)malloc(sizeof(*camera));
        glm_vec3_copy(eye, camera->eye);
        glm_vec3_copy(center, camera->center);
        glm_vec3_copy(up, camera->up);
        glm_vec2_copy(screenSize, camera->screenSize);
        scene->addCamera3D(camera);
        // Two overlapping light cubes covering both model planes
        // planeGap=5 matches initInstanceDataModelDualPlane; scale=6 → 60 units covers 50-unit model spread
        scene->initLightsDualPlane(&scene->getVolumetricLight(0), 5.0f, 6.0f, 1);

        scene->addModel(load_obj_to_geom_object(PREPEND_ABSOLUTE_PATH("assets/models/viking_room.obj")));
        scene->initInstanceDataModelDualPlane(&scene->getModel(0), MODEL_INSTANCE_COUNT);

        scene->init();

        // Replace the flat layout from init() with a preorder hierarchy of
        // 100 groups arranged as a 10x10 grid on the XY plane. Each group
        // is a light root (scale=1, 10x10x10 world AABB) parenting 200
        // model children (10x20 grid at 0.3 spacing → ~3x6 unit local
        // extent, safely inside the light's bounding volume).
        //
        //   Total: 100 groups × 200 models = 20000 model instances
        //          + 100 light instances
        //          = 20100 entities in the preorder transform array
        //
        // The group grid spans ±90 units on X and Y, so at typical camera
        // angles most groups fall entirely outside the frustum and their
        // subtrees are bulk-skipped by frustumCullScene in O(1).
        scene->buildAndApplyGroupedSpecTree(
            /*groupsPerSide*/ 10,
            /*modelsPerGroupX*/ 10,
            /*modelsPerGroupY*/ 20,
            /*groupSpacing*/ 20.0f,
            /*modelSpacing*/ 0.3f,
            /*lightScale*/ 1.0f);

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
                    // F key: parent light 0 instance 0 to model 0 instance 0 (cross-type attach)
                    if (event.key.keysym.scancode == SDL_SCANCODE_F)
                    {
                        SCENE *s = scene->getScene();
                        if (s->modelsManager.modelLen > 0 && s->lightManager.lightLen > 0)
                        {
                            uint32_t modelTIdx = s->modelsManager.pDenseModelInstances[0].transformIdx;
                            uint32_t lightTIdx = s->lightManager.pDenseLightInstances[0].transformIdx;
                            transformSetParent(&s->transformManager, lightTIdx, modelTIdx);
                            transformMarkDirty(&s->transformManager, lightTIdx);
                            std::cout << "[demo] Light 0 inst 0 parented to Model 0 inst 0" << std::endl;
                        }
                    }
                    // G key: unparent light 0 instance 0 (cross-type detach)
                    if (event.key.keysym.scancode == SDL_SCANCODE_G)
                    {
                        SCENE *s = scene->getScene();
                        if (s->lightManager.lightLen > 0)
                        {
                            uint32_t lightTIdx = s->lightManager.pDenseLightInstances[0].transformIdx;
                            transformClearParent(&s->transformManager, lightTIdx);
                            transformMarkDirty(&s->transformManager, lightTIdx);
                            std::cout << "[demo] Light 0 inst 0 unparented (root)" << std::endl;
                        }
                    }
                    // Arrow keys: move model 0 instance 0 transform (cross-type parenting demo)
                    if (event.key.keysym.scancode == SDL_SCANCODE_UP)
                    {
                        SCENE *s = scene->getScene();
                        if (s->modelsManager.modelLen > 0)
                        {
                            uint32_t tIdx = s->modelsManager.pDenseModelInstances[0].transformIdx;
                            TransformCold *node = &s->transformManager.pTransformsCold[tIdx];
                            vec3 delta = {0.0f, 0.0f, 1.0f};
                            transformTranslate(node, delta);
                            transformMarkDirty(&s->transformManager, tIdx);
                        }
                    }
                    if (event.key.keysym.scancode == SDL_SCANCODE_DOWN)
                    {
                        SCENE *s = scene->getScene();
                        if (s->modelsManager.modelLen > 0)
                        {
                            uint32_t tIdx = s->modelsManager.pDenseModelInstances[0].transformIdx;
                            TransformCold *node = &s->transformManager.pTransformsCold[tIdx];
                            vec3 delta = {0.0f, 0.0f, -1.0f};
                            transformTranslate(node, delta);
                            transformMarkDirty(&s->transformManager, tIdx);
                        }
                    }
                    if (event.key.keysym.scancode == SDL_SCANCODE_LEFT)
                    {
                        SCENE *s = scene->getScene();
                        if (s->modelsManager.modelLen > 0)
                        {
                            uint32_t tIdx = s->modelsManager.pDenseModelInstances[0].transformIdx;
                            TransformCold *node = &s->transformManager.pTransformsCold[tIdx];
                            vec3 delta = {-1.0f, 0.0f, 0.0f};
                            transformTranslate(node, delta);
                            transformMarkDirty(&s->transformManager, tIdx);
                        }
                    }
                    if (event.key.keysym.scancode == SDL_SCANCODE_RIGHT)
                    {
                        SCENE *s = scene->getScene();
                        if (s->modelsManager.modelLen > 0)
                        {
                            uint32_t tIdx = s->modelsManager.pDenseModelInstances[0].transformIdx;
                            TransformCold *node = &s->transformManager.pTransformsCold[tIdx];
                            vec3 delta = {1.0f, 0.0f, 0.0f};
                            transformTranslate(node, delta);
                            transformMarkDirty(&s->transformManager, tIdx);
                        }
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
                // Benchmark: fixed camera sequence + partial random dirty load.
                // Overrides interactive WASD/mouse-look so the profile output
                // corresponds to a reproducible scenario schedule.
                benchApplyCamera((uint32_t)frameCount, scene->getCamera3D());
                benchApplyRandomDirty(scene->getScene());

                // Render and present frame
                if (!resizing)
                    engine->metaDrawFrame(window, (float)deltaTime);

                lastFrameTime = currentTime;
                frameCount++;
                if (frameCount >= MAX_FRAME_COUNT)
                {
                    running = 0;
                }

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

        // Flush stats for the final scenario before teardown.
        displayBenchEndScenario();

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
