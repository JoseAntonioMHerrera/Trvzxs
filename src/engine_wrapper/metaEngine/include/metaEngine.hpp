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

class MetaEngine {

    private:
        constexpr static auto freeDeviceManager = [](DeviceManager* dm){ free(dm);};
        constexpr static auto freeInstance = [](VkInstance* instance){ free(instance);};
        constexpr static auto freeDisplayManager = [](DisplayManager* dispM){ free(dispM);};
        MetaObject<DeviceManager, void(*)(DeviceManager*)> deviceManager;
        MetaObject<VkInstance, void(*)(VkInstance*)> vkInstance;
        MetaObject<DisplayManager, void(*)(DisplayManager*)> displayManager; //TODO: Only a graphics pipeline need displayManager, on/off rendering
        std::unique_ptr<Scene> scene;
        uint32_t currentSceneIndex = 0;

    public:

    // Bulk init of everything
    MetaEngine(SDL_Window* window, std::unique_ptr<Scene> scene){
        std::cout << "ESTO ES METAENGINE CONSTRUCTOR \n";
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
        dispM->graphicsPipelineLen = 3;
        dispM->graphicPipeline = (GraphicPipelineToSurface*) malloc(sizeof(*dispM->graphicPipeline)* dispM->graphicsPipelineLen);

        std::cout << "WOOP" << std::endl;
        // PIPELINES
        GraphicPipelineToSurface *gdp = &dispM->graphicPipeline[0];
        GraphicPipelineToSurface *particleGraphicPipeline = &dispM->graphicPipeline[1];
        GraphicPipelineToSurface *lightVolumeGraphicPipeline = &dispM->graphicPipeline[2];
        gdp->scene = scene->getScene();
        particleGraphicPipeline->scene = scene->getScene();
        lightVolumeGraphicPipeline->scene = scene->getScene();
        std::cout << "WOOP" << std::endl;
        dispM->graphicPipeline[0].swapchain = (GraphicSwapchain*) malloc(sizeof(*dispM->graphicPipeline->swapchain));
        dispM->graphicPipeline[1].computeP = (ComputePipeline*) malloc(sizeof(*dispM->graphicPipeline[1].computeP));
        std::cout << "WOOP" << std::endl;
        // SWAPCHAIN
        GraphicSwapchain *dsm = (GraphicSwapchain*) dispM->graphicPipeline->swapchain;
        GraphicSwapchain *dsmParticle = dsm;
        GraphicSwapchain *dsmLightVolume = dsm;
        std::cout << "WOOP" << std::endl;
        // VULKAN BUFFERS
        DeviceBufferManager *devicebmModelGraphicPipeline;
        DeviceBufferManager *devicebmMParticleComputePipeline;
        DeviceBufferManager *devicebmParticleGraphicPipeline;
        DeviceBufferManager *devicebmVolumetricGraphicPipeline;
        gdp->deviceBuffer = (DeviceBufferManager*) malloc(sizeof(*gdp->deviceBuffer));
        particleGraphicPipeline->deviceBuffer = (DeviceBufferManager*) malloc(sizeof(*particleGraphicPipeline->deviceBuffer));
        particleGraphicPipeline->computeP->deviceBuffer = (DeviceBufferManager*) malloc(sizeof(*particleGraphicPipeline->computeP->deviceBuffer));
        lightVolumeGraphicPipeline->deviceBuffer = (DeviceBufferManager*) malloc(sizeof(*lightVolumeGraphicPipeline->deviceBuffer));

        std::cout << "WOOP" << std::endl;
        // COMMAND POOLS
        gdp->commandPool = (CommandPools*) malloc(sizeof(*gdp->commandPool));
        particleGraphicPipeline->commandPool = (CommandPools*) malloc(sizeof(*particleGraphicPipeline->commandPool));
        lightVolumeGraphicPipeline->commandPool = (CommandPools*) malloc(sizeof(*lightVolumeGraphicPipeline->commandPool));
        particleGraphicPipeline->computeP->commandPool = particleGraphicPipeline->commandPool;


        std::cout << "WOOP" << std::endl;

        //gdp->scene->modelsManager.pModel = scene.get()->getModels().data();


        //printScene_(gdp->geometry->pScene, gdp->geometry->sceneLen);
        std::cout << "RETURNED PRINT SCENE" << std::endl;

        this->scene = std::move(scene);
        particleGraphicPipeline->maxFrames = MAX_FRAMES_IN_FLIGHT;
        lightVolumeGraphicPipeline->maxFrames = MAX_FRAMES_IN_FLIGHT;
        gdp->maxFrames = MAX_FRAMES_IN_FLIGHT;
        dsm->scImagesLen = gdp->maxFrames;
        dsmParticle->scImagesLen = particleGraphicPipeline->maxFrames;
        dsmLightVolume->scImagesLen = lightVolumeGraphicPipeline->maxFrames;

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
        particleGraphicPipeline->swapchain = dsmParticle;
        lightVolumeGraphicPipeline->swapchain = dsmLightVolume;

        gdp->maxFrames = dsm->scImagesLen;
        lightVolumeGraphicPipeline->maxFrames = dsmLightVolume->scImagesLen;
        particleGraphicPipeline->maxFrames = dsmParticle->scImagesLen;
        std::cout << "NUMERO DE FRAMES " << gdp->maxFrames << '\n';
        std::cout << "RETORNO CREATESWAPCHAIN" << '\n';
        createMsaaColorResources(ld->device, pd, dsm);
        std::cout << "RETORNO msaacolorresources" << '\n';
        createMsaaDepthResources(ld->device, pd, dsm); // Pipeline grafica de geometria con multisampling
        std::cout << "RETORNO msaadepthresources" << '\n'; // Pipeline grafica de geometria con multisampling
        createDepthImageResources(pd, ld, dispM->graphicPipeline->swapchain); // Pipeline grafica de geometria sin multisampling
        printf("RETORNO DEPTHIMAGE\n");
        createGeometryBufferResources(pd, ld, dispM->graphicPipeline->swapchain);
        printf("RETORNO GeometryBufferResources\n");
        createNormalsBufferResources(pd, ld, dispM->graphicPipeline->swapchain);
        std::cout << "RETORNO normalsBuffer" << '\n';
        createRenderPass(pd, dispM->graphicPipeline[0].swapchain, ld->device, gdp); // Render pass composicion de una o mas pipeline graficas
        //TODO refactor the initialization of swapchain data, with default data for depth resources
        //createParticleRenderPass(pd, particleGraphicPipeline->swapchain, ld->device, particleGraphicPipeline);
        particleGraphicPipeline->renderPass = gdp->renderPass;
        lightVolumeGraphicPipeline->renderPass = gdp->renderPass;
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
        std::cout << "RETORNO FRAMEBUFFERS" << '\n';
        createGraphicsCommandPool(pd,ld->device,gdp);
                std::cout << "RETORNO FRAMEBUFFERS" << '\n';
        createTransferQueueCommandPool(pd,ld->device, gdp);
                std::cout << "RETORNO FRAMEBUFFERS" << '\n';
        createComputeQueueCommandPool(pd,ld->device,gdp);
                std::cout << "RETORNO FRAMEBUFFERS" << '\n';
        //createGraphicsCommandPool(pd,ld->device, particleGraphicPipeline);
        particleGraphicPipeline->commandPool = gdp->commandPool;
        lightVolumeGraphicPipeline->commandPool = gdp->commandPool;
        particleGraphicPipeline->computeP->commandPool = gdp->commandPool;
        std::cout << "RETORNO TRANSFERPOOL" << '\n';
        createCommandBuffer(gdp, &ld->device, gdp->maxFrames);
        createCommandBuffer(lightVolumeGraphicPipeline, &ld->device, lightVolumeGraphicPipeline->maxFrames);
                std::cout << "RETORNO PRIMER COMMAN BUFFER CREATION" << '\n';
        createCommandBufferWithCompute(particleGraphicPipeline, &ld->device, gdp->maxFrames);
                std::cout << "RETORNO SEGUNDO COMMAN BUFFER CREATION 2" << '\n';
        VkDeviceSize textureSize;
        uint32_t width;
        uint32_t height;
        createTextureImage( pd,  ld, gdp, &gdp->texture);
        std::cout << "RETORNO createTextureImage" << '\n';

        createTextureImageView(dispM->graphicPipeline->swapchain, ld->device, gdp->texture, gdp->texture->mipLevels);
        std::cout << "RETORNO createTextureImageView" << '\n';
        gdp->deviceBuffer->ubo = (UniformBufferManager*) malloc(sizeof(*gdp->deviceBuffer->ubo));
        particleGraphicPipeline->computeP->deviceBuffer->ubo = (UniformBufferManager*) malloc(sizeof(*gdp->deviceBuffer->ubo));
        lightVolumeGraphicPipeline->deviceBuffer->ubo = (UniformBufferManager*) malloc(sizeof(*lightVolumeGraphicPipeline->deviceBuffer->ubo));
        createModelInstanceTransformationsUniformBuffersMapped(pd,ld, gdp->maxFrames, gdp->deviceBuffer->ubo, gdp->scene->modelsManager.modelInstanceLen);
        createModelInstanceTransformationsUniformBuffersMapped(pd,ld, lightVolumeGraphicPipeline->maxFrames,
        lightVolumeGraphicPipeline->deviceBuffer->ubo, lightVolumeGraphicPipeline->scene->lightManager.lightInstanceLen);
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
        createDescriptorSet(gdp->maxFrames, ld->device, gdp, particleGraphicPipeline, lightVolumeGraphicPipeline, gdp->texture);
        std::cout << "RETORNO DESCRIPTOR SET" << '\n';
        particleGraphicPipeline->dsl = gdp->dsl;
        particleGraphicPipeline->descriptorPool = gdp->descriptorPool;
        particleGraphicPipeline->descriptorSet = gdp->descriptorSet;
        lightVolumeGraphicPipeline->descriptorSet = gdp->descriptorSet;
        lightVolumeGraphicPipeline->descriptorPool = gdp->descriptorPool;
        lightVolumeGraphicPipeline->dsl = gdp->dsl;
        std::cout << "RETORNO SETLAYOUT" << '\n';




        createModelsGraphicPipeline(gdp, dispM, &ld->device);
        std::cout << "RETORNO GRAPHIC PIPELINE FLAG METAENGINE" << '\n';
        createParticleGraphicPipeline(particleGraphicPipeline, dispM, &ld->device);
        std::cout << "RETORNO PARTICLE GRAPHIC PIPELINE FLAG METAENGINE" << '\n';
        createLightVolumeGraphicPipeline(lightVolumeGraphicPipeline, dispM, &ld->device);
        std::cout << "RETORNO LIGHT VOLUME GRAPHIC PIPELINE FLAG METAENGINE" << '\n';
        //gdp->computeP = (ComputePipeline*) (sizeof(*gdp->computeP));
        particleGraphicPipeline->computeP->pipelineLayout = particleGraphicPipeline->pipelineLayout;
        std::cout << "RETORNO GRPAHICSPIPLINE" << '\n';
        createParticleComputePipeline(particleGraphicPipeline->computeP, ld->device);
        std::cout << "RETORNO COMPUTEPIPELINE" << '\n';
        std::cout << "RETORNO COMMANDBUFFER" << '\n';
        createSyncObjects(gdp, &ld->device);
        createSyncObjectsForGraphicAndCompute(particleGraphicPipeline, &ld->device);
        std::cout << "RETORNO SYNC" << '\n';


        createGeometryBuffer(pd,ld,gdp->deviceBuffer,gdp->scene, gdp->commandPool);
        std::cout << "RETORNO GEOMETRYBUFFER MODEL" << '\n';



        assert(gdp->scene > 0);
        createGeometryIndiceBuffer(pd, ld,gdp->deviceBuffer,gdp->scene, gdp->commandPool);
        std::cout << "RETORNOs INDICE MODEL" << '\n';
        createLightVolumeVertexBuffer(pd,ld,lightVolumeGraphicPipeline->deviceBuffer,lightVolumeGraphicPipeline->scene, lightVolumeGraphicPipeline->commandPool);
        std::cout << "RETORNO GEOMETRYBUFFER MODEL" << '\n';



        assert(gdp->scene > 0);
        createLightVolumeIndiceBuffers(pd, ld,lightVolumeGraphicPipeline->deviceBuffer,lightVolumeGraphicPipeline->scene, lightVolumeGraphicPipeline->commandPool);
        std::cout << "RETORNOs INDICE MODEL" << '\n';
        dm->logCallback = createDebugCallbackForVulkan(dm->instance);
        dispM->currentFrame = 0;
        std::cout << "Init vulkan ended." << std::endl;
        this->deviceManager = MetaObject<DeviceManager,void(*)(DeviceManager*)>(dm, freeDeviceManager);
        this->displayManager = MetaObject<DisplayManager,void(*)(DisplayManager*)>(dispM, freeDisplayManager) ;
        this->vkInstance = MetaObject<VkInstance,void(*)(VkInstance*)>(*instance, freeInstance);
    };

    void firstFrameInitialPass(){
        DeviceManager* deviceManager = this->deviceManager.getStruct();
        firstFrameInitialPassCompute(deviceManager->pDevice, deviceManager->lDevice[0],// TODO hay que usar deviceManager->deviceIndex y gpuIndex
        deviceManager->lDevice[0]->displayManager,
        &deviceManager->lDevice[0]->displayManager->graphicPipeline[0],
                &deviceManager->lDevice[0]->displayManager->graphicPipeline[1],
        deviceManager->lDevice[0]->device);
    };

    void metaDrawFrame(SDL_Window* window, float deltaTime){
        DeviceManager* deviceManager = this->deviceManager.getStruct();
        Camera3D camera;
        drawFrame(deviceManager->pDevice, deviceManager->lDevice[0],// TODO hay que usar deviceManager->deviceIndex y gpuIndex
        deviceManager->lDevice[0]->displayManager,
        &deviceManager->lDevice[0]->displayManager->graphicPipeline[0],
                &deviceManager->lDevice[0]->displayManager->graphicPipeline[1],
                &deviceManager->lDevice[0]->displayManager->graphicPipeline[2],
        deviceManager->lDevice[0]->device, window, deltaTime, camera);
        //std::cout << "RETORNAMOS EL CONTROL DESDE DRAW FRAME" << std::endl;
    }
};
