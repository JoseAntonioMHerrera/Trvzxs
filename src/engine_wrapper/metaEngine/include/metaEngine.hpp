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
    #include "ssboBuffer.h"
    #include "swapchainManager.h"
    #include "image.h"
    #include "commandPool.h"
    #include "deviceManagment.h"
    #include "geometryPipelineTypes.h"
    #include "computePipeline.h"
    #include "textureManager.h"
    #include "descriptor.h"
    #include "scene.h"
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
        std::unique_ptr<Scene<T, U, V>> scene;
        uint32_t currentSceneIndex = 0;

    public:

    // Bulk init of everything
    MetaEngine(SDL_Window* window, std::unique_ptr<Scene<T, U, V>> scene){
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


        // DISPLAY MANAGER

        std::cout << "WOOP" << std::endl;
        DisplayManager *dispM = (DisplayManager*) malloc(sizeof(DisplayManager));
        dispM->graphicsPipelineLen = 2;
        dispM->graphicPipeline = (GraphicPipelineToSurface*) malloc(sizeof(*dispM->graphicPipeline)* dispM->graphicsPipelineLen);

        std::cout << "WOOP" << std::endl;
        // PIPELINES
        GraphicPipelineToSurface *gdp = &dispM->graphicPipeline[0];
        GraphicPipelineToSurface *particleGraphicPipeline = &dispM->graphicPipeline[1];

        std::cout << "WOOP" << std::endl;
        dispM->graphicPipeline[0].swapchain = (GraphicSwapchain*) malloc(sizeof(*dispM->graphicPipeline->swapchain));
        dispM->graphicPipeline[1].computeP = (ComputePipeline*) malloc(sizeof(*dispM->graphicPipeline[1].computeP));
        std::cout << "WOOP" << std::endl;
        // SWAPCHAIN
        GraphicSwapchain *dsm = (GraphicSwapchain*) dispM->graphicPipeline->swapchain;
        GraphicSwapchain *dsmParticle = dsm;
        std::cout << "WOOP" << std::endl;
        // VULKAN BUFFERS
        DeviceBufferManager *devicebmModelGraphicPipeline;
        DeviceBufferManager *devicebmMParticleComputePipeline;
        DeviceBufferManager *devicebmParticleGraphicPipeline;
        gdp->deviceBuffer = (DeviceBufferManager*) malloc(sizeof(*gdp->deviceBuffer));
        particleGraphicPipeline->deviceBuffer = (DeviceBufferManager*) malloc(sizeof(*gdp->deviceBuffer));
        particleGraphicPipeline->computeP->deviceBuffer = (DeviceBufferManager*) malloc(sizeof(*gdp->computeP->deviceBuffer));

        std::cout << "WOOP" << std::endl;
        // COMMAND POOLS
        gdp->commandPool = (CommandPools*) malloc(sizeof(*gdp->commandPool));
        particleGraphicPipeline->commandPool = (CommandPools*) malloc(sizeof(*gdp->commandPool));
        particleGraphicPipeline->computeP->commandPool = particleGraphicPipeline->commandPool;

        std::cout << "WOOP" << std::endl;

        gdp->geometry = scene.get()->getStruct();


        //printScene_(gdp->geometry->pScene, gdp->geometry->sceneLen);
        std::cout << "RETURNED PRINT SCENE" << std::endl;

        this->scene = std::move(scene);
        particleGraphicPipeline->maxFrames = MAX_FRAMES_IN_FLIGHT;
        gdp->maxFrames = MAX_FRAMES_IN_FLIGHT;
        dsm->scImagesLen = gdp->maxFrames;
        dsmParticle->scImagesLen = particleGraphicPipeline->maxFrames;

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
        createSwapChain(pd, ld, dispM, gdp, window);
        particleGraphicPipeline->swapchain = gdp->swapchain;

        gdp->maxFrames = dsm->scImagesLen;
        particleGraphicPipeline->maxFrames = dsmParticle->scImagesLen;
        std::cout << "NUMERO DE FRAMES " << gdp->maxFrames << '\n';
        std::cout << "RETORNO CREATESWAPCHAIN" << '\n';
        createMsaaColorResources(ld->device, pd, dsm);
        std::cout << "RETORNO msaacolorresources" << '\n';
        createMsaaDepthResources(ld->device, pd, dsm);
        std::cout << "RETORNO msaadepthresources" << '\n';
        createDepthImageResources(pd, ld, dispM->graphicPipeline->swapchain);
        std::cout << "RETORNO DEPTIMAGE" << '\n';
        createRenderPass(pd, dispM->graphicPipeline[0].swapchain, ld->device, gdp);
        //TODO refactor the initialization of swapchain data, with default data for depth resources
        //createParticleRenderPass(pd, particleGraphicPipeline->swapchain, ld->device, particleGraphicPipeline);
        particleGraphicPipeline->renderPass = gdp->renderPass;
        //dispM->graphicPipeline[1].renderPass = dispM->graphicPipeline[0].renderPass;
        std::cout << "RETORNO RENDERPASS" << '\n';
        dsm->scImageViewLen = dsm->scImagesLen;
        std::cout << "RETORNO RENDERPASS" << '\n';


        for(uint32_t i=0; i < ld->displayManager->graphicPipeline->swapchain->scImagesLen; i++){
            ld->displayManager->graphicPipeline[0].swapchain->p_scImageView[i] =
            createImageView(dsm, ld->device, dsm->currentSurfaceFormat->format, ld->displayManager->graphicPipeline->swapchain->p_scImages[i], 1, VK_IMAGE_ASPECT_COLOR_BIT);
        }
        std::cout << "RETORNO IMAGEVIEWS" << '\n';

        createFrameBuffers(dsm,gdp->renderPass, &ld->device);
        std::cout << "RETORNO FRAMEBUFFERS" << '\n';

        devicebmModelGraphicPipeline = gdp->deviceBuffer;

        createGraphicsCommandPool(pd,ld->device,gdp);
        createGraphicsCommandPool(pd,ld->device, particleGraphicPipeline);
        createTransferQueueCommandPool(pd,ld->device, particleGraphicPipeline);
        createTransferQueueCommandPool(pd,ld->device, gdp);
        particleGraphicPipeline->computeP->commandPool = particleGraphicPipeline->commandPool;

        std::cout << "RETORNO COMMANDPOOLD" << '\n';
        createTransferQueueCommandPool(pd,ld->device,gdp);
        std::cout << "RETORNO TRANSFERPOOL" << '\n';
        createComputeQueueCommandPool(pd,ld->device,particleGraphicPipeline->computeP);
        std::cout << "RETORNO TRANSFERPOOL" << '\n';
        createCommandBuffer(gdp, &ld->device, gdp->maxFrames);
                std::cout << "RETORNO PRIMER COMMAN BUFFER CREATION" << '\n';
        createCommandBufferWithCompute(particleGraphicPipeline, &ld->device, gdp->maxFrames);
                std::cout << "RETORNO PRIMER COMMAN BUFFER CREATION 2" << '\n';
        VkDeviceSize textureSize;
        uint32_t width;
        uint32_t height;
        createTextureImage( pd,  ld, gdp, &gdp->texture);
        std::cout << "RETORNO createTextureImage" << '\n';

        createTextureImageView(dispM->graphicPipeline->swapchain, ld->device, gdp->texture, gdp->texture->mipLevels);
        std::cout << "RETORNO createTextureImageView" << '\n';
        gdp->deviceBuffer->ubo = (UniformBufferManager*) malloc(sizeof(*gdp->deviceBuffer->ubo));
        particleGraphicPipeline->computeP->deviceBuffer->ubo = (UniformBufferManager*) malloc(sizeof(*gdp->deviceBuffer->ubo));
        createUniformBuffersMapped(pd,ld, gdp->maxFrames, gdp->deviceBuffer->ubo);
        std::cout << "RETORNO UNIFORMBUFFERS MAPPED TRANSFORM MATRIX" << '\n';
        createDeltaTimeUniformBuffers(pd,ld, gdp->maxFrames, particleGraphicPipeline->computeP->deviceBuffer->ubo);
        std::cout << "RETORNO UNIFORMBUFFERS DELTA TIME" << '\n';
        std::cout << "!!!!!! " << gdp->maxFrames << '\n';
        particleGraphicPipeline->computeP->deviceBuffer->ssbos = (SSBOBufferManager*) malloc(sizeof(*gdp->deviceBuffer->ssbos));
        createSSBOBuffers(pd,ld, particleGraphicPipeline->maxFrames, particleGraphicPipeline->computeP->deviceBuffer->ssbos, particleGraphicPipeline->computeP->commandPool);
        std::cout << "RETORNO SSBOBUFFERS" << '\n';
        createDescriptorPool(gdp->maxFrames, ld->device, gdp);
        createDescriptorSetLayout(ld, gdp, gdp->maxFrames);
        std::cout << "RETORNO SETLAYOUT" << '\n';
        std::cout << "RETORNO DESCRIPTOR POOL" << '\n';
        createDescriptorSet(gdp->maxFrames, ld->device, gdp, particleGraphicPipeline, gdp->texture);
        std::cout << "RETORNO DESCRIPTOR SET" << '\n';
        particleGraphicPipeline->dsl = gdp->dsl;
        particleGraphicPipeline->descriptorPool = gdp->descriptorPool;
        particleGraphicPipeline->descriptorSet = gdp->descriptorSet;
        std::cout << "RETORNO SETLAYOUT" << '\n';
        createModelsGraphicPipeline(gdp, dispM, &ld->device);
        std::cout << "RETORNO GRAPHIC PIPELINE" << '\n';
        createParticlesGraphicPipeline(particleGraphicPipeline, dispM, &ld->device);
        std::cout << "RETORNO PARTICLE GRAPHIC PIPELINE" << '\n';
        //gdp->computeP = (ComputePipeline*) (sizeof(*gdp->computeP));
        particleGraphicPipeline->computeP->pipelineLayout = particleGraphicPipeline->pipelineLayout;
        std::cout << "RETORNO GRPAHICSPIPLINE" << '\n';
        createParticleComputePipeline(particleGraphicPipeline->computeP, ld->device);
        std::cout << "RETORNO COMPUTEPIPELINE" << '\n';
        std::cout << "RETORNO COMMANDBUFFER" << '\n';
        createSyncObjects(gdp, &ld->device);
        createSyncObjectsForGraphicAndCompute(particleGraphicPipeline, &ld->device);
        std::cout << "RETORNO SYNC" << '\n';
        createGeometryBuffer(pd,ld,gdp->deviceBuffer,gdp->geometry, gdp->commandPool);
        std::cout << "RETORNO GEOMETRYBUFFER" << '\n';

        std::cout << "RETORNO GEOMETRYBUFFER" << '\n';
        //ssbo particle buffer
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

    void firstFrameInitialPass(){
        DeviceManager* deviceManager = this->deviceManager.getStruct();
        firstFrameInitialPassCompute(deviceManager->pDevice, deviceManager->lDevice[0],// TODO hay que usar deviceManager->deviceIndex and gpuIndex
        deviceManager->lDevice[0]->displayManager,
        &deviceManager->lDevice[0]->displayManager->graphicPipeline[0],
                &deviceManager->lDevice[0]->displayManager->graphicPipeline[1],
        deviceManager->lDevice[0]->device);
    };

    void metaDrawFrame(SDL_Window* window, float deltaTime){
        DeviceManager* deviceManager = this->deviceManager.getStruct();
        std::cout << "ESTE ES EL DELTA TIME EN C++ " << deltaTime << std::endl;
        drawFrame(deviceManager->pDevice, deviceManager->lDevice[0],// TODO hay que usar deviceManager->deviceIndex and gpuIndex
        deviceManager->lDevice[0]->displayManager,
        &deviceManager->lDevice[0]->displayManager->graphicPipeline[0],
                &deviceManager->lDevice[0]->displayManager->graphicPipeline[1],
        deviceManager->lDevice[0]->device, window, deltaTime);
        //std::cout << "RETORNAMOS EL CONTROL DESDE DRAW FRAME" << std::endl;
    }
};
