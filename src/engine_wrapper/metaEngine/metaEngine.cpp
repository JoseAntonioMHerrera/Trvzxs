#include "metaEngine.hpp"

#include "initContext.hpp"
#include "initBindless.hpp"
#include "initBindlessRegistration.hpp"
#include "initDescriptors.hpp"
#include "initDeviceFeatures.hpp"
#include "initDisplay.hpp"
#include "initGeometryBuffers.hpp"
#include "initInstance.hpp"
#include "initPipelines.hpp"
#include "initRenderGraph.hpp"
#include "initStorageBuffers.hpp"
#include "initSwapchainResources.hpp"
#include "initTextures.hpp"
#include "initUniformBuffers.hpp"

#include <cstdlib>

extern "C"
{
#include "main_vulkan.h"
}

void MetaEngine::initVkInstance(SDL_Window *window)
{
    (void)window;
    DeviceManager *dm = (DeviceManager *)malloc(sizeof(DeviceManager));
    this->deviceManager =
        MetaObject<DeviceManager, void (*)(DeviceManager *)>(
            dm, freeDeviceManager);
}

void MetaEngine::bulkInitVulkanWithScene(SDL_Window *window, Scene *scene)
{
    MetaInitContext _initCtx{};
    _initCtx.dm = this->deviceManager.getStruct();
    metaInitInstance(_initCtx, window);
    metaInitDeviceFeatures(_initCtx); // A11 bindless hook (no-op today)

    metaInitDisplay(_initCtx, scene, window);
    this->scene.setScene(scene->getScene());

    metaInitSwapchainResources(_initCtx, window);
    metaInitBindless(_initCtx); // A11 — needs ctx.ld/ctx.pd populated by swapchain init

    metaInitTextures(_initCtx);
    metaInitUniformBuffers(_initCtx);
    metaInitStorageBuffers(_initCtx);
    metaInitGeometryBuffers(_initCtx);
    // metaInitDescriptors(_initCtx); // DORMANT A11 — explicit descriptor path, preserved
    metaInitBindlessRegistration(_initCtx); // A11 — register texture+SSBOs, alloc per-pass sets

    metaInitRenderGraph(_initCtx,
                        &this->renderGraph,
                        &this->depthPrepassData,
                        &this->modelPassData,
                        &this->lightPassData,
                        &this->particlePassData,
                        &this->computePassData);
    metaInitPipelines(_initCtx, &this->renderGraph);

    this->displayManager =
        MetaObject<DisplayManager, void (*)(DisplayManager *)>(
            _initCtx.dispM, freeDisplayManager);
    this->scene.addCamera3D(scene->getCamera3D());
}

void MetaEngine::metaCleanup()
{
    DeviceManager            *dm    = this->deviceManager.getStruct();
    LogicalDevice            *ld    = dm->lDevice[0];
    DisplayManager           *dispM = ld->displayManager;
    GraphicPipelineToSurface *gdp   = &dispM->graphicPipeline[0];
    cleanup(dm, ld, dispM, gdp, dm->logCallback);
}

void MetaEngine::metaDrawFrame(SDL_Window *window, float deltaTime)
{
    DeviceManager *dm = this->deviceManager.getStruct();
    Camera3D camera = *this->scene.getCamera3D();

    drawFrame(
        dm->pDevice,
        dm->lDevice[0], // TODO: use deviceManager->deviceIndex / gpuIndex
        dm->lDevice[0]->displayManager,
        &dm->lDevice[0]->displayManager->graphicPipeline[0],
        &dm->lDevice[0]->displayManager->graphicPipeline[1],
        &dm->lDevice[0]->displayManager->graphicPipeline[2],
        dm->lDevice[0]->device, window, deltaTime, camera,
        &this->renderGraph);
}

void MetaEngine::metaRecreateSwapchain(SDL_Window *window)
{
    DeviceManager *dm = this->deviceManager.getStruct();
    recreateSwapChain(
        dm->pDevice,
        dm->lDevice[0],
        window,
        &this->renderGraph);
}
