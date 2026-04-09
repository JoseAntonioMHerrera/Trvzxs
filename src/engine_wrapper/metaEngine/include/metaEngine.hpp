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
#include "passCallbacks.h"
#include "passData.h"
#include "renderGraph.h"
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
    RenderGraph renderGraph;
    DepthPrepassData depthPrepassData;
    ModelPassData modelPassData;
    LightPassData lightPassData;
    ParticlePassData particlePassData;
    ComputePassData computePassData;

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
        createGBufferSampler(ld->device,
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
            (UniformBufferManager *)calloc(1, sizeof(*gdp->deviceBuffer->pUbo));
        particleGraphicPipeline->computeP->deviceBuffer->pUbo =
            (UniformBufferManager *)calloc(1, sizeof(*gdp->deviceBuffer->pUbo));
        lightVolumeGraphicPipeline->deviceBuffer->pUbo =
            (UniformBufferManager *)calloc(1,
                sizeof(*lightVolumeGraphicPipeline->deviceBuffer->pUbo));
        createModelInstanceTransformationsUniformBuffersMapped(
            pd, ld, gdp->maxFrames, gdp->deviceBuffer->pUbo,
            gdp->scene->modelsManager.modelInstanceCapacity);
        createLightInstanceTransformationsUniformBuffersMapped(
            pd, ld, lightVolumeGraphicPipeline->maxFrames,
            lightVolumeGraphicPipeline->deviceBuffer->pUbo,
            lightVolumeGraphicPipeline->scene->lightManager
                .lightInstanceCapacity);

        // Per-frame globals UBO (binding 9) — shared across all pipelines
        gdp->pGlobalsUbo = (UniformBufferManager *)calloc(1, sizeof(*gdp->pGlobalsUbo));
        createFrameGlobalsUniformBuffersMapped(pd, ld, gdp->maxFrames, gdp->pGlobalsUbo);

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

        // Pipeline creation deferred until after rgCompile — see below.
        // The render graph creates single-subpass render passes; pipelines
        // must be created with those render passes (not the old multi-subpass
        // one) to ensure subpass compatibility.
        // Compute pipeline also deferred (needs particle pipelineLayout).
        createSyncObjects(gdp, &ld->device);
        createSyncObjects(particleGraphicPipeline, &ld->device);
        createFrameSync(&ld->frameSync, ld->device, gdp->maxFrames);
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

        // === RENDER GRAPH SETUP ===
        rgInit(&this->renderGraph, ld->device, pd,
               dsm->currentExtent2D.width, dsm->currentExtent2D.height,
               dsm->scImagesLen);
        this->renderGraph.debugLog = true;
        rgSetQueues(&this->renderGraph,
                    *ld->queueManager.graphicsQueue,
                    *ld->queueManager.computeQueue,
                    pd->queueFamilyIndices.graphicsFamily,
                    pd->queueFamilyIndices.computeFamily);

        // Register external resources
        uint32_t rColor = rgAddExternalResource(&this->renderGraph, (RGResource){
            .type       = RG_IMAGE,
            .format     = dsm->currentSurfaceFormat->format,
            .samples    = VK_SAMPLE_COUNT_1_BIT,
            .sizeMode   = RG_SIZE_SWAPCHAIN,
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .image      = dsm->p_scImages[0],
            .imageView  = dsm->p_scImageView[0],
            .perFrameImageViews = dsm->p_scImageView,
            .isExternal = true,
        });

        uint32_t rDepth = rgAddExternalResource(&this->renderGraph, (RGResource){
            .type       = RG_IMAGE,
            .format     = findDepthImageFormat(pd->gpu),
            .samples    = VK_SAMPLE_COUNT_1_BIT,
            .sizeMode   = RG_SIZE_SWAPCHAIN,
            .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
            .image      = dsm->depthImage.common->image,
            .imageView  = dsm->depthImage.common->imageView,
            .isExternal = true,
        });

        uint32_t rGBuffer = rgAddExternalResource(&this->renderGraph, (RGResource){
            .type       = RG_IMAGE,
            .format     = findGeometryBufferFormat(pd->gpu),
            .samples    = VK_SAMPLE_COUNT_1_BIT,
            .sizeMode   = RG_SIZE_SWAPCHAIN,
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .image      = dsm->gBuffer->image,
            .imageView  = dsm->gBuffer->imageView,
            .isExternal = true,
        });

        uint32_t rNormals = rgAddExternalResource(&this->renderGraph, (RGResource){
            .type       = RG_IMAGE,
            .format     = findNormalsBufferFormat(pd->gpu),
            .samples    = VK_SAMPLE_COUNT_1_BIT,
            .sizeMode   = RG_SIZE_SWAPCHAIN,
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .image      = dsm->nBuffer->image,
            .imageView  = dsm->nBuffer->imageView,
            .isExternal = true,
        });

        // Pass 0: Depth prepass — vertex-only, populates depth buffer
        uint32_t pDepthPrepass = rgAddPass(&this->renderGraph, RG_PASS_GRAPHICS);
        rgPassSetName(&this->renderGraph, pDepthPrepass, RG_PASS_NAME_DEPTH_PREPASS);
        rgPassAddOutput(&this->renderGraph, pDepthPrepass, (RGResourceUsage){
            .resourceIndex = rDepth,
            .layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            .accessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            .stageMask  = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
                          VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
            .access     = RG_WRITE,
        });

        this->depthPrepassData = (DepthPrepassData){
            .depthPipeline = depthPrepassGraphicPipeline,
            .modelPipeline = gdp,
            .dm            = dispM,
        };
        rgPassSetCallback(&this->renderGraph, pDepthPrepass, executeDepthPrepass, &this->depthPrepassData);

        // Pass 1: Models — reads depth (prepass populated it), writes G-buffer + normals
        uint32_t pModels = rgAddPass(&this->renderGraph, RG_PASS_GRAPHICS);
        rgPassSetName(&this->renderGraph, pModels, RG_PASS_NAME_MODELS);
        rgPassAddOutput(&this->renderGraph, pModels, (RGResourceUsage){
            .resourceIndex = rGBuffer,
            .layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .accessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .stageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .access     = RG_WRITE,
        });
        rgPassAddOutput(&this->renderGraph, pModels, (RGResourceUsage){
            .resourceIndex = rNormals,
            .layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .accessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .stageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .access     = RG_WRITE,
        });
        // Depth: input triggers a barrier from the prepass (depth write → read),
        // output makes the render graph create a pDepthStencilAttachment so the
        // hardware depth test works. The pipeline has depthWriteEnable=FALSE so
        // no actual depth writes occur despite the output declaration.
        rgPassAddInput(&this->renderGraph, pModels, (RGResourceUsage){
            .resourceIndex = rDepth,
            .layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
            .accessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
            .stageMask  = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
            .access     = RG_READ,
        });
        rgPassAddOutput(&this->renderGraph, pModels, (RGResourceUsage){
            .resourceIndex = rDepth,
            .layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            .accessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
            .stageMask  = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
                          VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
            .access     = RG_READ,
        });

        this->modelPassData = (ModelPassData){
            .pipeline = gdp,
            .dm       = dispM,
        };
        rgPassSetCallback(&this->renderGraph, pModels, executeModelPass, &this->modelPassData);

        // Pass 1: Lights — reads G-buffer + normals, writes color
        uint32_t pLights = rgAddPass(&this->renderGraph, RG_PASS_GRAPHICS);
        rgPassSetName(&this->renderGraph, pLights, RG_PASS_NAME_LIGHTS);
        rgPassAddInput(&this->renderGraph, pLights, (RGResourceUsage){
            .resourceIndex = rGBuffer,
            .layout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .accessMask = VK_ACCESS_SHADER_READ_BIT,
            .stageMask  = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            .access     = RG_READ,
        });
        rgPassAddInput(&this->renderGraph, pLights, (RGResourceUsage){
            .resourceIndex = rNormals,
            .layout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .accessMask = VK_ACCESS_SHADER_READ_BIT,
            .stageMask  = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            .access     = RG_READ,
        });
        rgPassAddInput(&this->renderGraph, pLights, (RGResourceUsage){
            .resourceIndex = rDepth,
            .layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
            .accessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
            .stageMask  = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
            .access     = RG_READ,
        });
        rgPassAddOutput(&this->renderGraph, pLights, (RGResourceUsage){
            .resourceIndex = rColor,
            .layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .accessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .stageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .access     = RG_WRITE,
        });

        this->lightPassData = (LightPassData){
            .pipeline = lightVolumeGraphicPipeline,
            .dm       = dispM,
        };
        rgPassSetCallback(&this->renderGraph, pLights, executeLightPass, &this->lightPassData);

        // Pass 2: Particles — reads + writes color
        uint32_t pParticles = rgAddPass(&this->renderGraph, RG_PASS_GRAPHICS);
        rgPassSetName(&this->renderGraph, pParticles, RG_PASS_NAME_PARTICLES);
        rgPassAddInput(&this->renderGraph, pParticles, (RGResourceUsage){
            .resourceIndex = rColor,
            .layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .accessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT,
            .stageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .access     = RG_READ,
        });
        rgPassAddOutput(&this->renderGraph, pParticles, (RGResourceUsage){
            .resourceIndex = rColor,
            .layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .accessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .stageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .access     = RG_WRITE,
        });

        this->particlePassData = (ParticlePassData){
            .pipeline = particleGraphicPipeline,
            .dm       = dispM,
        };
        rgPassSetCallback(&this->renderGraph, pParticles, executeParticlePass, &this->particlePassData);

        // Resources 4-5: Particle SSBO double buffers (external, per-frame)
        SSBOBufferManager* ssbos = particleGraphicPipeline->computeP->deviceBuffer->ssbos;
        uint32_t rSSBOPrev = rgAddExternalResource(&this->renderGraph, (RGResource){
            .type       = RG_BUFFER,
            .buffer     = ssbos->buffers[0],
            .bufferSize = VK_WHOLE_SIZE,
            .isExternal = true,
        });
        uint32_t rSSBOCurr = rgAddExternalResource(&this->renderGraph, (RGResource){
            .type       = RG_BUFFER,
            .buffer     = ssbos->buffers[1],
            .bufferSize = VK_WHOLE_SIZE,
            .isExternal = true,
        });

        // Pass 3: Particle compute — reads prev SSBO, writes curr SSBO
        uint32_t pParticleCompute = rgAddPass(&this->renderGraph, RG_PASS_COMPUTE);
        rgPassSetName(&this->renderGraph, pParticleCompute, RG_PASS_NAME_PARTICLE_COMPUTE);
        rgPassSetQueue(&this->renderGraph, pParticleCompute, RG_QUEUE_COMPUTE);
        rgPassAddInput(&this->renderGraph, pParticleCompute, (RGResourceUsage){
            .resourceIndex = rSSBOPrev,
            .accessMask    = VK_ACCESS_SHADER_READ_BIT,
            .stageMask     = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            .access        = RG_READ,
        });
        rgPassAddOutput(&this->renderGraph, pParticleCompute, (RGResourceUsage){
            .resourceIndex = rSSBOCurr,
            .accessMask    = VK_ACCESS_SHADER_WRITE_BIT,
            .stageMask     = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            .access        = RG_WRITE,
        });

        this->computePassData = (ComputePassData){
            .pipeline  = particleGraphicPipeline,
            .dm        = dispM,
            .pd        = pd,
            .ld        = ld,
            .deltaTime = &this->renderGraph.deltaTime,
        };
        rgPassSetCallback(&this->renderGraph, pParticleCompute, executeParticleComputePass, &this->computePassData);

        // Particle render pass also reads the current SSBO (vertex input)
        rgPassAddInput(&this->renderGraph, pParticles, (RGResourceUsage){
            .resourceIndex = rSSBOCurr,
            .accessMask    = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,
            .stageMask     = VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
            .access        = RG_READ,
        });

        // Compile: topological sort + barrier generation + render pass creation
        rgCompile(&this->renderGraph);

        // Create graphics pipelines AFTER rgCompile so they use the render
        // graph's single-subpass render passes (subpass 0) instead of the
        // old multi-subpass render pass.

        // Depth prepass pipeline — uses the render graph's depth-only render pass
        depthPrepassGraphicPipeline->renderPass = &this->renderGraph.passes[pDepthPrepass].renderPass;
        depthPrepassGraphicPipeline->dsl = gdp->dsl;
        depthPrepassGraphicPipeline->descriptorSet = gdp->descriptorSet;
        createDepthPrepassGraphicsPipeline(depthPrepassGraphicPipeline, dispM, &ld->device);

        gdp->renderPass = &this->renderGraph.passes[pModels].renderPass;
        createModelsGraphicPipeline(gdp, dispM, &ld->device);

        lightVolumeGraphicPipeline->renderPass = &this->renderGraph.passes[pLights].renderPass;
        createLightVolumeGraphicPipeline(lightVolumeGraphicPipeline, dispM, &ld->device);

        particleGraphicPipeline->renderPass = &this->renderGraph.passes[pParticles].renderPass;
        createParticleGraphicPipeline(particleGraphicPipeline, dispM, &ld->device);

        // Compute pipeline needs particle pipelineLayout (created above)
        particleGraphicPipeline->computeP->pipelineLayout =
            particleGraphicPipeline->pipelineLayout;
        createParticleComputePipeline(particleGraphicPipeline->computeP,
                                      ld->device);

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
            deviceManager->lDevice[0]->device, window, deltaTime, camera,
            &this->renderGraph);
        // std::cout << "RETORNAMOS EL CONTROL DESDE DRAW FRAME" <<
        // std::endl;
    }

    void metaRecreateSwapchain(SDL_Window *window)
    {
        DeviceManager *deviceManager = this->deviceManager.getStruct();
        recreateSwapChain(
            deviceManager->pDevice,
            deviceManager->lDevice[0],
            window,
            &this->renderGraph);
    }
};
