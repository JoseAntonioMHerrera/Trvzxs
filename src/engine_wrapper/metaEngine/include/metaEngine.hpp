#pragma once
#include "metaObject.hpp"
#include "scene.hpp"
#include <functional>
extern "C"
{
#include "buffer.h"
#include "commandPool.h"
#include "computePipeline.h"
#include "descriptor.h"
#include "deviceManagment.h"
#include "geometryPipelineTypes.h"
#include "image.h"
#include "main_vulkan.h"
#include "scene.h"
#include "ssboBuffer.h"
#include "swapchainManager.h"
#include "textureManager.h"
#include "vertex.h"
#include <SDL2/SDL.h>
#include <vulkan/vulkan.h>
}

class MetaEngine
{

private:
    constexpr static auto freeDeviceManager = [](DeviceManager *dm)
    { free(dm); };
    constexpr static auto freeInstance = [](VkInstance *instance)
    { free(instance); };
    constexpr static auto freeDisplayManager = [](DisplayManager *dispM)
    { free(dispM); };
    MetaObject<DeviceManager, void (*)(DeviceManager *)> deviceManager;
    MetaObject<VkInstance, void (*)(VkInstance *)> vkInstance;
    MetaObject<DisplayManager, void (*)(DisplayManager *)>
        displayManager; // TODO: Only a graphics pipeline need displayManager,
                        // on/off rendering
    Scene scene;
    uint32_t currentSceneIndex = 0;

public:
    void initVkInstance(SDL_Window *window)
    {
        std::cout << "ESTO ES METAENGINE CONSTRUCTOR \n";
        DeviceManager *dm = (DeviceManager *)malloc(
            sizeof(DeviceManager)); // Init for the general state object of
                                    // the render
        std::cout << "inittrazosengine flag 1" << '\n';
        dm->lDevice = (LogicalDevice **)malloc(sizeof(*dm->lDevice));
        (*dm->lDevice) = (LogicalDevice *)malloc(sizeof(**dm->lDevice) * 1);
        dm->pDevice = (PhysicalDevice *)malloc(sizeof(dm->pDevice));
        dm->gpuIndex = 0;
        dm->deviceIndex = 0;
        VkInstance **instance = (VkInstance **)malloc(sizeof(VkInstance *));
        createInstance(&dm->instance, window);
        this->deviceManager =
            MetaObject<DeviceManager, void (*)(DeviceManager *)>(
                dm, freeDeviceManager);
        std::cout << "RETORNO CREATEINSTANCE" << '\n';
    }

    void bulkInitVulkanWithScene(SDL_Window *window, Scene *scene)
    {
        std::cout << "ESTO ES METAENGINE CONSTRUCTOR \n";
        DeviceManager *dm = this->deviceManager.getStruct(); // Init for the general state object of
                                                             // the render
        std::cout << "inittrazosengine flag 1" << '\n';
        dm->lDevice = (LogicalDevice **)malloc(sizeof(*dm->lDevice));
        (*dm->lDevice) = (LogicalDevice *)malloc(sizeof(**dm->lDevice) * 1);
        dm->pDevice = (PhysicalDevice *)malloc(sizeof(dm->pDevice));
        dm->gpuIndex = 0;
        dm->deviceIndex = 0;
        VkInstance **instance = (VkInstance **)malloc(sizeof(VkInstance *));
        createInstance(&dm->instance, window);
        std::cout << "RETORNO CREATEINSTANCE" << '\n';

        // DISPLAY MANAGER

        std::cout << "WOOP" << std::endl;
        DisplayManager *dispM =
            (DisplayManager *)malloc(sizeof(DisplayManager));
        dispM->graphicsPipelineLen = 4; // 0 models 1 particles 2 lihts 3 depth prepass
        dispM->graphicPipeline = (GraphicPipelineToSurface *)malloc(
            sizeof(*dispM->graphicPipeline) * dispM->graphicsPipelineLen);

        // PIPELINES
        GraphicPipelineToSurface *gdp = &dispM->graphicPipeline[0];
        GraphicPipelineToSurface *particleGraphicPipeline =
            &dispM->graphicPipeline[1];
        GraphicPipelineToSurface *lightVolumeGraphicPipeline =
            &dispM->graphicPipeline[2];
        GraphicPipelineToSurface *depthPrepassGraphicPipeline =
            &dispM->graphicPipeline[3];
        gdp->scene = scene->getScene();
        printSceneModels(&gdp->scene->modelsManager);

        particleGraphicPipeline->scene = scene->getScene();
        lightVolumeGraphicPipeline->scene = scene->getScene();
        depthPrepassGraphicPipeline->scene = scene->getScene();

        dispM->graphicPipeline[0].swapchain = (GraphicSwapchain *)malloc(
            sizeof(*dispM->graphicPipeline->swapchain));
        GraphicSwapchain *dsm =
            (GraphicSwapchain *)dispM->graphicPipeline->swapchain;
        GraphicSwapchain *dsmParticle = dsm;
        GraphicSwapchain *dsmLightVolume = dsm;
        GraphicSwapchain *dsmDepthPrePass = dsm;
        particleGraphicPipeline->computePipelineCount = 1;
        particleGraphicPipeline->computeP = (ComputePipeline *)malloc( // TODO change order models -> light ->particles
            sizeof(*particleGraphicPipeline->computeP));

        depthPrepassGraphicPipeline->computePipelineCount = 2;
        depthPrepassGraphicPipeline->computeP = (ComputePipeline *)malloc( // TODO change order models -> light ->particles
            sizeof(*depthPrepassGraphicPipeline->computeP) * depthPrepassGraphicPipeline->computePipelineCount);
        ComputePipeline fillHiZComputeP = depthPrepassGraphicPipeline->computeP[0];
        ComputePipeline oclComputeP = depthPrepassGraphicPipeline->computeP[1];
        // SWAPCHAIN SHARED

        // VULKAN BUFFERS

        gdp->deviceBuffer =
            (DeviceBufferManager *)calloc(1, sizeof(*gdp->deviceBuffer));
        particleGraphicPipeline->deviceBuffer = (DeviceBufferManager *)calloc(
            1, sizeof(*particleGraphicPipeline->deviceBuffer));
        particleGraphicPipeline->computeP->deviceBuffer = (DeviceBufferManager *)calloc(
            1, sizeof(*particleGraphicPipeline->computeP->deviceBuffer));
        particleGraphicPipeline->deviceBuffer = (DeviceBufferManager *)calloc(
            1, sizeof(*particleGraphicPipeline->deviceBuffer));
        lightVolumeGraphicPipeline->deviceBuffer = (DeviceBufferManager *)calloc(
            1, sizeof(*lightVolumeGraphicPipeline->deviceBuffer));
        depthPrepassGraphicPipeline->deviceBuffer = (DeviceBufferManager *)calloc(
            1, sizeof(*lightVolumeGraphicPipeline->deviceBuffer));

        for (uint32_t compIndex = 0; compIndex < particleGraphicPipeline->computePipelineCount; compIndex++)
        {
            particleGraphicPipeline->computeP[compIndex].deviceBuffer = (DeviceBufferManager *)calloc(
                1, sizeof(*particleGraphicPipeline->computeP[compIndex].deviceBuffer));
        }
        for (uint32_t compIndex = 0; compIndex < depthPrepassGraphicPipeline->computePipelineCount; compIndex++)
        {
            depthPrepassGraphicPipeline->computeP[compIndex].deviceBuffer = (DeviceBufferManager *)calloc(
                1, sizeof(*depthPrepassGraphicPipeline->computeP[compIndex].deviceBuffer));
        }

        // COMMAND POOLS
        gdp->commandPool =
            (CommandPools *)malloc(sizeof(*gdp->commandPool));
        particleGraphicPipeline->commandPool = (CommandPools *)malloc(
            sizeof(*particleGraphicPipeline->commandPool));
        lightVolumeGraphicPipeline->commandPool = (CommandPools *)malloc(
            sizeof(*lightVolumeGraphicPipeline->commandPool));
        particleGraphicPipeline->computeP->commandPool =
            particleGraphicPipeline->commandPool;
        depthPrepassGraphicPipeline->commandPool = (CommandPools *)malloc(
            sizeof(*lightVolumeGraphicPipeline->commandPool));

        for (uint32_t compIndex = 0; compIndex < particleGraphicPipeline->computePipelineCount; compIndex++)
        {
            particleGraphicPipeline->computeP[compIndex].commandPool = (CommandPools *)malloc(sizeof(CommandPools));
        }
        for (uint32_t compIndex = 0; compIndex < depthPrepassGraphicPipeline->computePipelineCount; compIndex++)
        {
            depthPrepassGraphicPipeline->computeP[compIndex].commandPool = (CommandPools *)malloc(sizeof(CommandPools));
        }
        // gdp->scene->modelsManager.pModel =
        // scene.get()->getModels().data();

        printSceneModels(&gdp->scene->modelsManager);

        this->scene.setScene(scene->getScene());
        printSceneModels(&this->scene.getScene()->modelsManager);

        particleGraphicPipeline->maxFrames = MAX_FRAMES_IN_FLIGHT;
        lightVolumeGraphicPipeline->maxFrames = MAX_FRAMES_IN_FLIGHT;
        gdp->maxFrames = MAX_FRAMES_IN_FLIGHT;
        depthPrepassGraphicPipeline->maxFrames = MAX_FRAMES_IN_FLIGHT;

        dsm->scImagesLen = gdp->maxFrames;
        dsmParticle->scImagesLen = particleGraphicPipeline->maxFrames;
        dsmLightVolume->scImagesLen = lightVolumeGraphicPipeline->maxFrames;
        dsmDepthPrePass->scImagesLen = depthPrepassGraphicPipeline->maxFrames;

        createSDLSurface(dispM, dm->instance, window);

        initDeviceManager(dm->instance, dm, dispM);

        PhysicalDevice *pd = &dm->pDevice[dm->gpuIndex];
        LogicalDevice *ld = dm->lDevice[dm->deviceIndex];

        physicalDeviceTest(pd->gpu);

        if (VK_FALSE == isDeviceValid(ld->device))
        {
            throw std::runtime_error("isDeviceValid returns VK_FALSE");
        }
        createSwapChain(pd, ld, dispM, gdp, window);
        particleGraphicPipeline->swapchain = dsmParticle; // TODO demasiao confuso, cambiar a dsmGdp
        lightVolumeGraphicPipeline->swapchain = dsmLightVolume;
        depthPrepassGraphicPipeline->swapchain = dsmDepthPrePass;

        gdp->maxFrames = dsm->scImagesLen;
        lightVolumeGraphicPipeline->maxFrames = dsmLightVolume->scImagesLen;
        particleGraphicPipeline->maxFrames = dsmParticle->scImagesLen;
        depthPrepassGraphicPipeline->maxFrames = dsmDepthPrePass->scImagesLen;

        createMsaaColorResources(ld, pd, dsm);

        createMsaaDepthResources(
            ld, pd,
            dsm); // Pipeline grafica de geometria con multisampling

        createDepthImageResources(
            pd, ld,
            dispM->graphicPipeline
                ->swapchain); // Pipeline grafica de geometria sin
                              // multisampling
        /*createHiZImageResources(
            pd, ld,
            depthPrepassGraphicPipeline);*/
        createGeometryBufferResources(pd, ld,
                                      dispM->graphicPipeline->swapchain);
        createNormalsBufferResources(pd, ld,
                                     dispM->graphicPipeline->swapchain);
        std::cout << "RETORNO normalsBuffer" << '\n';
        createRenderPass(
            pd, dispM->graphicPipeline[0].swapchain, ld->device,
            gdp); // Render pass composicion de una o mas pipeline graficas
        particleGraphicPipeline->renderPass = gdp->renderPass;
        lightVolumeGraphicPipeline->renderPass = gdp->renderPass;

        /*createPreDepthRenderPass(pd, depthPrepassGraphicPipeline->swapchain, ld->device,
                                 depthPrepassGraphicPipeline);*/

        dsm->scImageViewLen = dsm->scImagesLen; // todo ??

        for (uint32_t i = 0;
             i <
             ld->displayManager->graphicPipeline->swapchain->scImagesLen;
             i++)
        {
            ld->displayManager->graphicPipeline[0]
                .swapchain->p_scImageView[i] = createImageView(
                dsm, ld->device, dsm->currentSurfaceFormat->format,
                ld->displayManager->graphicPipeline->swapchain
                    ->p_scImages[i],
                1, VK_IMAGE_ASPECT_COLOR_BIT);
        }
        std::cout << "RETORNO IMAGEVIEWS" << '\n';

        createFrameBuffers(dsm, gdp->renderPass, &ld->device);
        std::cout << "RETORNO FRAMEBUFFERS" << '\n';

        std::cout << "RETORNO FRAMEBUFFERS" << '\n';
        createGraphicsCommandPool(pd, ld->device, gdp);
        std::cout << "RETORNO FRAMEBUFFERS" << '\n';
        createTransferQueueCommandPool(pd, ld->device, gdp);
        std::cout << "RETORNO FRAMEBUFFERS" << '\n';
        createComputeQueueCommandPool(pd, ld->device, gdp);
        std::cout << "RETORNO FRAMEBUFFERS" << '\n';
        // createGraphicsCommandPool(pd,ld->device,
        // particleGraphicPipeline);
        particleGraphicPipeline->commandPool = gdp->commandPool;
        lightVolumeGraphicPipeline->commandPool = gdp->commandPool;
        depthPrepassGraphicPipeline->commandPool = gdp->commandPool; // TODO por ahora todos comparten el mism commandPool
        for (uint32_t compIndex = 0; compIndex < particleGraphicPipeline->computePipelineCount; compIndex++)
        {
            particleGraphicPipeline->computeP[compIndex].commandPool = particleGraphicPipeline->commandPool;
        }

        for (uint32_t compIndex = 0; compIndex < depthPrepassGraphicPipeline->computePipelineCount; compIndex++)
        {
            depthPrepassGraphicPipeline->computeP[compIndex].commandPool = depthPrepassGraphicPipeline->commandPool;
        }
        std::cout << "RETORNO TRANSFERPOOL" << '\n';
        createCommandBuffer(gdp, &ld->device, gdp->maxFrames);
        createCommandBuffer(lightVolumeGraphicPipeline, &ld->device,
                            lightVolumeGraphicPipeline->maxFrames);
        std::cout << "RETORNO PRIMER COMMAN BUFFER CREATION" << '\n';
        createCommandBufferWithCompute(particleGraphicPipeline, &ld->device,
                                       gdp->maxFrames);
        /*createCommandBufferWithCompute(depthPrepassGraphicPipeline, &ld->device,
                                       gdp->maxFrames);*/
        std::cout << "RETORNO SEGUNDO COMMAN BUFFER CREATION 2" << '\n';
        VkDeviceSize textureSize;
        uint32_t width;
        uint32_t height;
        createTextureImage(pd, ld, gdp, &gdp->texture);
        std::cout << "RETORNO createTextureImage" << '\n';

        createTextureImageView(dispM->graphicPipeline->swapchain,
                               ld->device, gdp->texture,
                               gdp->texture->mipLevels);
        std::cout << "RETORNO createTextureImageView" << '\n';
        gdp->deviceBuffer->pUbo =
            (UniformBufferManager *)malloc(sizeof(*gdp->deviceBuffer->pUbo));
        particleGraphicPipeline->computeP->deviceBuffer->pUbo =
            (UniformBufferManager *)malloc(sizeof(*gdp->deviceBuffer->pUbo));
        lightVolumeGraphicPipeline->deviceBuffer->pUbo =
            (UniformBufferManager *)malloc(
                sizeof(*lightVolumeGraphicPipeline->deviceBuffer->pUbo));
        createModelInstanceTransformationsUniformBuffersMapped(
            pd, ld, gdp->maxFrames, gdp->deviceBuffer->pUbo,
            gdp->scene->modelsManager.modelInstanceLen);
        createLightInstanceTransformationsUniformBuffersMapped(
            pd, ld, lightVolumeGraphicPipeline->maxFrames,
            lightVolumeGraphicPipeline->deviceBuffer->pUbo,
            lightVolumeGraphicPipeline->scene->lightManager
                .lightInstanceCount);

        std::cout
            << "RETORNO UNIFORMBUFFERS MAPPED TRANSFORM MATRIX"
            << '\n';
        createDeltaTimeUniformBuffers(
            pd, ld, gdp->maxFrames,
            particleGraphicPipeline->computeP->deviceBuffer->pUbo);
        std::cout << "RETORNO UNIFORMBUFFERS DELTA TIME" << '\n';
        std::cout << "!!!!!! " << gdp->maxFrames << '\n';
        particleGraphicPipeline->computeP->deviceBuffer->ssbos =
            (SSBOBufferManager *)malloc(sizeof(*gdp->deviceBuffer->ssbos));
        // oclComputeP.deviceBuffer->ssbos =
        //     (SSBOBufferManager *)malloc(sizeof(*gdp->deviceBuffer->ssbos));
        createParticleSSBOBuffer(
            pd, ld, particleGraphicPipeline->maxFrames,
            particleGraphicPipeline->computeP->deviceBuffer->ssbos,
            particleGraphicPipeline->computeP->commandPool,
            particleGraphicPipeline->scene);
        /*createOclCullingSSBOBuffer(
            pd, ld, depthPrepassGraphicPipeline->maxFrames,
            oclComputeP.deviceBuffer->ssbos,
            oclComputeP.commandPool,
            depthPrepassGraphicPipeline->scene);*/
        createParticleBuffer(pd, ld, particleGraphicPipeline->deviceBuffer, particleGraphicPipeline->scene,
                             particleGraphicPipeline->commandPool);
        createParticleIndiceBuffers(
            pd, ld, particleGraphicPipeline->deviceBuffer,
            particleGraphicPipeline->scene,
            particleGraphicPipeline->commandPool);
        std::cout << "RETORNO PARTICLE SSBO BUFFERS\n"
                  << std::endl;
        lightVolumeGraphicPipeline->deviceBuffer->ssbos =
            (SSBOBufferManager *)malloc(sizeof(*lightVolumeGraphicPipeline->deviceBuffer->ssbos));
        createReadbackLightPass1SSBOBuffers( // TODO: add preprocessing for switching readbackbuffers debugging
            pd, ld, lightVolumeGraphicPipeline->maxFrames,
            lightVolumeGraphicPipeline->deviceBuffer->ssbos,
            lightVolumeGraphicPipeline->commandPool);
        std::cout << "RETORNO LIGHT SSBOBUFFERS" << '\n';
        createDescriptorPool(gdp->maxFrames, ld->device, gdp);
        createDescriptorSetLayout(ld, gdp, gdp->maxFrames);
        std::cout << "RETORNO SETLAYOUT" << '\n';
        std::cout << "RETORNO DESCRIPTOR POOL" << '\n';
        createDescriptorSet(gdp->maxFrames, ld->device, gdp,
                            particleGraphicPipeline,
                            lightVolumeGraphicPipeline, gdp->texture);
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
        createParticleGraphicPipeline(particleGraphicPipeline, dispM,
                                      &ld->device);
        std::cout << "RETORNO PARTICLE GRAPHIC PIPELINE FLAG METAENGINE"
                  << '\n';
        createLightVolumeGraphicPipeline(lightVolumeGraphicPipeline, dispM,
                                         &ld->device);
        // createDepthPrepassGraphicsPipeline(depthPrepassGraphicPipeline, dispM,
        //                                    &ld->device);
        std::cout << "RETORNO LIGHT VOLUME GRAPHIC PIPELINE FLAG METAENGINE"
                  << '\n';
        // gdp->computeP = (ComputePipeline*) (sizeof(*gdp->computeP));
        particleGraphicPipeline->computeP->pipelineLayout =
            particleGraphicPipeline->pipelineLayout;
        std::cout << "RETORNO GRPAHICSPIPLINE" << '\n';
        createParticleComputePipeline(particleGraphicPipeline->computeP,
                                      ld->device);
        /*createOcclusionCullingComputePipeline(&oclComputeP,
                                              ld->device);*/
        std::cout << "RETORNO COMPUTEPIPELINE" << '\n';
        std::cout << "RETORNO COMMANDBUFFER" << '\n';
        createSyncObjects(gdp, &ld->device);
        createSyncObjectsForGraphicAndCompute(particleGraphicPipeline,
                                              &ld->device);
        std::cout << "RETORNO SYNC" << '\n';

        createGeometryBuffer(pd, ld, gdp->deviceBuffer, gdp->scene,
                             gdp->commandPool);
        std::cout << "RETORNO GEOMETRYBUFFER MODEL" << '\n';

        assert(gdp->scene > 0);
        createGeometryIndiceBuffer(pd, ld, gdp->deviceBuffer, gdp->scene,
                                   gdp->commandPool);
        std::cout << "RETORNOs INDICE MODEL" << '\n';
        createLightVolumeVertexBuffer(
            pd, ld, lightVolumeGraphicPipeline->deviceBuffer,
            lightVolumeGraphicPipeline->scene,
            lightVolumeGraphicPipeline->commandPool);
        std::cout << "RETORNO GEOMETRYBUFFER MODEL" << '\n';

        assert(gdp->scene > 0);
        if (lightVolumeGraphicPipeline->scene->lightManager.packedLightIndex.itemsLen > 0)
            createLightVolumeIndiceBuffers(
                pd, ld, lightVolumeGraphicPipeline->deviceBuffer,
                lightVolumeGraphicPipeline->scene,
                lightVolumeGraphicPipeline->commandPool);
        std::cout << "RETORNOs INDICE MODEL" << '\n';
        dm->logCallback = createDebugCallbackForVulkan(dm->instance);
        dispM->currentFrame = 0;
        std::cout << "Init vulkan ended." << std::endl;
        this->displayManager =
            MetaObject<DisplayManager, void (*)(DisplayManager *)>(
                dispM, freeDisplayManager);
        this->vkInstance = MetaObject<VkInstance, void (*)(VkInstance *)>(
            *instance, freeInstance);
        this->scene.addCamera3D(scene->getCamera3D());
    }
    // TODO: Bulk init of everything, refactor onDemand
    MetaEngine()
    {
    };

    void firstFrameInitialPass()
    {
        DeviceManager *deviceManager = this->deviceManager.getStruct();
        firstFrameInitialPassCompute(
            deviceManager->pDevice,
            deviceManager
                ->lDevice[0], // TODO hay que usar
                              // deviceManager->deviceIndex y gpuIndex
            deviceManager->lDevice[0]->displayManager,
            &deviceManager->lDevice[0]->displayManager->graphicPipeline[0],
            &deviceManager->lDevice[0]->displayManager->graphicPipeline[1],
            deviceManager->lDevice[0]->device);
    };

    void metaCleanup()
    {
        DeviceManager *dm = this->deviceManager.getStruct();
        LogicalDevice *ld = dm->lDevice[0];
        DisplayManager *dispM = ld->displayManager;
        GraphicPipelineToSurface *gdp = &dispM->graphicPipeline[0];
        cleanup(dm, ld, dispM, gdp, dm->logCallback);
    }

    void metaDrawFrame(SDL_Window *window, float deltaTime)
    {
        DeviceManager *deviceManager = this->deviceManager.getStruct();
        Camera3D camera = *this->scene.getCamera3D();

        drawFrame(
            deviceManager->pDevice,
            deviceManager
                ->lDevice[0], // TODO hay que usar
                              // deviceManager->deviceIndex y gpuIndex
            deviceManager->lDevice[0]->displayManager,
            &deviceManager->lDevice[0]->displayManager->graphicPipeline[0],
            &deviceManager->lDevice[0]->displayManager->graphicPipeline[1],
            &deviceManager->lDevice[0]->displayManager->graphicPipeline[2],
            deviceManager->lDevice[0]->device, window, deltaTime, camera);
        // std::cout << "RETORNAMOS EL CONTROL DESDE DRAW FRAME" <<
        // std::endl;
    }
};
