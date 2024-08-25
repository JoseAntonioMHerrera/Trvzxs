#pragma once
#include "metaObject.hpp"
#include "scene.hpp"
#include <functional>
extern "C" {
    #include "deviceManagment.h"
    #include <vulkan/vulkan.h>
    #include "main_vulkan.h"
    #include "vertex.h"
    #include "buffer.h"
    #include "swapchainManager.h"
    #include "commandPool.h"
    #include "deviceManagment.h"
    #include "geometryPipelineTypes.h"
    #include "textureManager.h"
    #include "descriptor.h"
    #include <SDL2/SDL.h>

}
template <typename T, template <typename> class U, typename V> // TODO MetaEngine<SCENE, GEOM_OBJECT, ...> define in C library an interface for every type defined
class MetaEngine {

    private:
        constexpr static auto freeDeviceManager = [](DeviceManager* dm){ free(dm);};
        constexpr static auto freeInstance = [](VkInstance* instance){ free(instance);};
        constexpr static auto freeDisplayManager = [](DisplayManager* dispM){ free(dispM);};
        MetaObject<DeviceManager, void(*)(DeviceManager*)> deviceManager;
        MetaObject<VkInstance, void(*)(VkInstance*)> vkInstance;
        MetaObject<DisplayManager, void(*)(DisplayManager*)> displayManager; //TODO: Only a graphics pipeline need displayManager, on/off rendering
        Scene<T, U, V> scene;
        uint32_t currentSceneIndex = 0;

    public:

    // Bulk init of everything
    MetaEngine(SDL_Window* window, Scene<T, U, V>& scene){
        DeviceManager* dm = (DeviceManager*) malloc(sizeof(DeviceManager)); // Init for the general state object of the render
        std::cout << "inittrazosengine flag 1" << '\n';
        dm->lDevice = (LogicalDevice**) malloc(sizeof(*dm->lDevice));
        (*dm->lDevice) = (LogicalDevice*) malloc(sizeof(**dm->lDevice) * 1 );
        dm->pDevice = (PhysicalDevice*) malloc(sizeof(dm->pDevice));
        dm->gpuIndex = 0;
        dm->deviceIndex = 0;
        VkInstance **instance =  (VkInstance**) malloc(sizeof(VkInstance*));
        createInstance(&dm->instance, window);
        std::cout << "RETORNO CREATEINSTANCE" << '\n';
        DisplayManager *dispM = (DisplayManager*) malloc(sizeof(DisplayManager));
        dispM->swapChainM = (DisplaySwapchainManager*) malloc(sizeof(*dispM->swapChainM));
        dispM->geomPipeline = (GeometryDisplayPipelineManager*) malloc(sizeof(*dispM->geomPipeline));
        DisplaySwapchainManager *dsm = (DisplaySwapchainManager*) dispM->swapChainM;

        GeometryDisplayPipelineManager *gdp = dispM->geomPipeline;
        DeviceBufferManager *devicebm;
        gdp->commandPool = (CommandPools*) malloc(sizeof(*gdp->commandPool));
        gdp->maxFrames = MAX_FRAMES_IN_FLIGHT;
        dsm->scImagesLen = gdp->maxFrames;
        createSDLSurface(dispM, dm->instance, window);
        std::cout << "RETORNO CREATESURFACE" << '\n';

        initDeviceManager(dm->instance, dm, dispM);
        std::cout << "RETORNOINITDEVICEMANAGER" << '\n';

        PhysicalDevice* pd = &dm->pDevice[dm->gpuIndex];
        LogicalDevice *ld = dm->lDevice[dm->deviceIndex];
        std::cout << "RETORNO FILLGEOMETRYDATA" << '\n';
        physicalDeviceTest(pd->gpu);
        if(VK_FALSE == isDeviceValid(ld->device)){
            throw std::runtime_error("isDeviceValid returns VK_FALSE");
        }



        createSwapChain(pd, ld, dispM, dsm, window);
        gdp->maxFrames = dsm->scImagesLen;
        std::cout << "NUMERO DE FRAMES " << gdp->maxFrames << '\n';
        std::cout << "RETORNO CREATESWAPCHAIN" << '\n';
        createRenderPass(dispM->swapChainM,ld->device,gdp);
        std::cout << "RETORNO RENDERPASS" << '\n';
        dsm->scImageViewLen = dsm->scImagesLen;
        for(uint32_t i=0; i < ld->displayManager->swapChainM->scImagesLen; i++){
            ld->displayManager->swapChainM->p_scImageView[i] =
                        createImageView(dsm, ld->device, dsm->currentSurfaceFormat->format, ld->displayManager->swapChainM->p_scImages[i]);
        }
        std::cout << "RETORNO IMAGEVIEWS" << '\n';
        createFrameBuffers(dsm,gdp->renderPass, &ld->device);
        std::cout << "RETORNO FRAMEBUFFERS" << '\n';
        gdp->deviceBuffer = (DeviceBufferManager*) malloc(sizeof(*gdp->deviceBuffer));
        devicebm = gdp->deviceBuffer;

        createGraphicsCommandPool(pd,ld->device,gdp);
        std::cout << "RETORNO COMMANDPOOLD" << '\n';
        createTransferQueueCommandPool(pd,ld->device,gdp);
        std::cout << "RETORNO TRANSFERPOOL" << '\n';
        createCommandBuffer(gdp, &ld->device, gdp->maxFrames);
        VkDeviceSize textureSize;
        uint32_t width;
        uint32_t height;
        createTextureImage( pd,  ld, gdp, &gdp->texture);
        std::cout << "RETORNO createTextureImage" << '\n';
        createTextureImageView(dispM->swapChainM, ld->device, gdp->texture);
        std::cout << "RETORNO createTextureImageView" << '\n';
        devicebm->ubo = (UniformBufferManager*) malloc(sizeof(*(gdp->deviceBuffer->ubo)));
        createUniformBuffers(pd,ld, gdp->maxFrames, gdp->deviceBuffer->ubo);
        std::cout << "RETORNO UNIFORMBUFFERS" << '\n';
        std::cout << "!!!!!! " << gdp->maxFrames << '\n';
        createDescriptorPool(gdp->maxFrames, ld->device, gdp);
        createDescriptorSetLayout(ld, gdp, gdp->maxFrames);
        std::cout << "RETORNO SETLAYOUT" << '\n';
        std::cout << "RETORNO DESCRIPTOR POOL" << '\n';
        createDescriptorSet(gdp->maxFrames, ld->device, gdp, gdp->texture);
        std::cout << "RETORNO SETLAYOUT" << '\n';
        createGraphicsPipeline(gdp, dispM, &ld->device);
        std::cout << "RETORNO GRPAHICSPIPLINE" << '\n';
        std::cout << "RETORNO COMMANDBUFFER" << '\n';
        createSyncObjects(gdp, &ld->device);
        std::cout << "RETORNO SYNC" << '\n';

        createGeometryBuffer(pd,ld,gdp->deviceBuffer,gdp->geometry, gdp->commandPool);
        std::cout << "RETORNO GEOMETRYBUFFER" << '\n';

        assert(&gdp->geometry->pScene[0]);
        createIndiceBuffer(pd, ld,gdp->deviceBuffer,gdp->geometry, gdp->commandPool);
        std::cout << "RETORNOs INDICE" << '\n';

        dm->logCallback = createDebugCallbackForVulkan(dm->instance);
        dispM->currentFrame = 0;
        std::cout << "Init vulkan ended." << std::endl;
        this->deviceManager = MetaObject<DeviceManager,void(*)(DeviceManager*)>(dm, freeDeviceManager);
        this->displayManager = MetaObject<DisplayManager,void(*)(DisplayManager*)>(dispM, freeDisplayManager) ;
        this->vkInstance = MetaObject<VkInstance,void(*)(VkInstance*)>(*instance, freeInstance);
    };
};
