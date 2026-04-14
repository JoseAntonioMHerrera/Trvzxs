#pragma once

#include "metaObject.hpp"
#include "scene.hpp"

extern "C"
{
#include "deviceManagment.h"
#include "passData.h"
#include "renderGraph.h"
#include "swapchainManager.h"
#include <SDL2/SDL.h>
#include <vulkan/vulkan.h>
}

class MetaEngine
{
private:
    constexpr static auto freeDeviceManager = [](DeviceManager *dm)
    { free(dm); };
    constexpr static auto freeDisplayManager = [](DisplayManager *dispM)
    { free(dispM); };

    MetaObject<DeviceManager, void (*)(DeviceManager *)> deviceManager;
    MetaObject<DisplayManager, void (*)(DisplayManager *)> displayManager;
    // TODO: Only a graphics pipeline needs displayManager, on/off rendering.

    Scene scene;
    uint32_t currentSceneIndex = 0;

    RenderGraph renderGraph;
    DepthPrepassData depthPrepassData;
    ModelPassData    modelPassData;
    LightPassData    lightPassData;
    ParticlePassData particlePassData;
    ComputePassData  computePassData;

public:
    MetaEngine() = default;

    // Minimal bootstrap: allocate the DeviceManager struct and take ownership.
    // Full dm/lDevice/pDevice allocation + createInstance happens in
    // bulkInitVulkanWithScene (via metaInitInstance).
    void initVkInstance(SDL_Window *window);

    // Monolithic init — delegates to the init/ helper modules in I5 phase
    // order: instance → device features → display → swapchain resources →
    // textures → UBOs → SSBOs → geometry buffers → descriptors →
    // render graph → pipelines.
    void bulkInitVulkanWithScene(SDL_Window *window, Scene *scene);

    void metaCleanup();
    void metaDrawFrame(SDL_Window *window, float deltaTime);
    void metaRecreateSwapchain(SDL_Window *window);
};
