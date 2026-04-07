#include "camera.h"
#include "computePipeline.h"
#include "deletionQueue.h"
#include "displayManager.h"
#include "timelineSync.h"
#include "particle.h"
#include "renderGraph.h"
#include "transformNode.h"
#include "transformations.h"
#include "mat4.h"
#include "affine.h"
#include <assert.h>
#include <time.h>
#include <stdio.h>

// ── Frame timing instrumentation ──────────────────────────────────────────────
#ifdef DEBUG
#define FRAME_TIMING_INTERVAL 120 // print averages every N frames

typedef struct FrameTiming
{
    double semaphoreWait;
    double deletionQueue;
    double ssboDescriptor;
    double acquireImage;
    double cmdRecord;
    double resolveTransforms;
    double uboGlobals;
    double uboModels;
    double uboLights;
    double submit;
    double present;
    double totalCpu;
} FrameTiming;

static FrameTiming g_accumTiming  = {0};
static uint32_t    g_timingFrames = 0;

static inline double timespecDiffUs(struct timespec* start, struct timespec* end)
{
    return (double)(end->tv_sec - start->tv_sec) * 1e6 + (double)(end->tv_nsec - start->tv_nsec) / 1e3;
}

static void accumulateTiming(FrameTiming* t)
{
    g_accumTiming.semaphoreWait    += t->semaphoreWait;
    g_accumTiming.deletionQueue    += t->deletionQueue;
    g_accumTiming.ssboDescriptor   += t->ssboDescriptor;
    g_accumTiming.acquireImage     += t->acquireImage;
    g_accumTiming.cmdRecord        += t->cmdRecord;
    g_accumTiming.resolveTransforms += t->resolveTransforms;
    g_accumTiming.uboGlobals       += t->uboGlobals;
    g_accumTiming.uboModels        += t->uboModels;
    g_accumTiming.uboLights        += t->uboLights;
    g_accumTiming.submit           += t->submit;
    g_accumTiming.present          += t->present;
    g_accumTiming.totalCpu         += t->totalCpu;
    g_timingFrames++;

    if (g_timingFrames >= FRAME_TIMING_INTERVAL)
    {
        double n = (double) g_timingFrames;
        printf("\n── Frame timing (avg over %u frames, µs) ──\n", g_timingFrames);
        printf("  semaphore wait   : %8.1f\n", g_accumTiming.semaphoreWait / n);
        printf("  deletion queue   : %8.1f\n", g_accumTiming.deletionQueue / n);
        printf("  ssbo descriptor  : %8.1f\n", g_accumTiming.ssboDescriptor / n);
        printf("  acquire image    : %8.1f\n", g_accumTiming.acquireImage / n);
        printf("  cmd record       : %8.1f\n", g_accumTiming.cmdRecord / n);
        printf("  resolveTransforms: %8.1f\n", g_accumTiming.resolveTransforms / n);
        printf("  ubo globals      : %8.1f\n", g_accumTiming.uboGlobals / n);
        printf("  ubo models       : %8.1f\n", g_accumTiming.uboModels / n);
        printf("  ubo lights       : %8.1f\n", g_accumTiming.uboLights / n);
        printf("  submit           : %8.1f\n", g_accumTiming.submit / n);
        printf("  present          : %8.1f\n", g_accumTiming.present / n);
        printf("  ─────────────────────────────\n");
        printf("  TOTAL CPU frame  : %8.1f  (%.2f ms)\n", g_accumTiming.totalCpu / n,
               g_accumTiming.totalCpu / n / 1000.0);
        printf("──────────────────────────────────────────\n\n");
        fflush(stdout);

        g_accumTiming  = (FrameTiming){0};
        g_timingFrames = 0;
    }
}

#define MARK_TIME(name) struct timespec name; clock_gettime(CLOCK_MONOTONIC, &name)
#endif // DEBUG
void cleanupSyncObjects(VkDevice device, GraphicPipelineToSurface* gdp)
{
    for (size_t i = 0; i < gdp->maxFrames; i++)
    {
        vkDestroySemaphore(device, gdp->renderEndFlag[i], NULL);
        vkDestroySemaphore(device, gdp->imgViewSemaphore[i], NULL);
    }
}

void createSDLSurface(DisplayManager* dm, VkInstance* instance, SDL_Window* window)
{
    SDL_bool result_surface_creation = SDL_Vulkan_CreateSurface(window, *instance, &dm->surface);
    assert(dm->surface);
    assert(*instance);

    assert(result_surface_creation == SDL_TRUE);
}


void updateSSBODescriptorSets(VkDevice device, GraphicPipelineToSurface* particleGraphics, uint32_t currentFrame,
                              uint32_t maxFrames)
{
    // Calculate previous frame index with wraparound
    uint32_t prevFrame = (currentFrame - 1 + maxFrames) % maxFrames;

    // Update SSBO descriptor bindings for previous and current frame
    VkDescriptorBufferInfo prevSSBOBufferInfo = {
        .buffer = particleGraphics->computeP->deviceBuffer->ssbos->buffers[prevFrame],
        .offset = 0,
        .range  = sizeof(ParticleInstance) * particleGraphics->scene->particleManager.packedParticleInstanceLen,
    };

    VkDescriptorBufferInfo currentSSBOBufferInfo = {
        .buffer = particleGraphics->computeP->deviceBuffer->ssbos->buffers[currentFrame],
        .offset = 0,
        .range  = sizeof(ParticleInstance) * particleGraphics->scene->particleManager.packedParticleInstanceLen,
    };

    // Create descriptor write updates for both SSBO bindings
    VkWriteDescriptorSet descriptorWrites[] = {
        {
            .sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet           = particleGraphics->descriptorSet[currentFrame],
            .dstBinding       = 2, // Previous frame SSBO binding
            .dstArrayElement  = 0,
            .descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount  = 1,
            .pBufferInfo      = &prevSSBOBufferInfo,
            .pImageInfo       = VK_NULL_HANDLE,
            .pTexelBufferView = VK_NULL_HANDLE,
        },
        {
            .sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet           = particleGraphics->descriptorSet[currentFrame],
            .dstBinding       = 3, // Current frame SSBO binding
            .dstArrayElement  = 0,
            .descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount  = 1,
            .pBufferInfo      = &currentSSBOBufferInfo,
            .pImageInfo       = VK_NULL_HANDLE,
            .pTexelBufferView = VK_NULL_HANDLE,
        },
    };

    // Update the descriptor sets
    vkUpdateDescriptorSets(device, 2, descriptorWrites, 0, VK_NULL_HANDLE);
}

void drawFrame(PhysicalDevice* pd, LogicalDevice* ld, DisplayManager* dm, GraphicPipelineToSurface* gdp,
               GraphicPipelineToSurface* particleGraphics, GraphicPipelineToSurface* lightGraphics, VkDevice device,
               SDL_Window* window, float deltaTime, Camera3D camera, RenderGraph* rg)
{
#ifdef DEBUG
    MARK_TIME(tFrameStart);
#endif

    FrameSync* sync = &ld->frameSync;

    // CPU pacing: wait for frame N-maxFrames to finish on the graphics timeline
    if (sync->graphicsValue >= sync->maxFramesInFlight)
    {
        uint64_t            waitValue = sync->graphicsValue - sync->maxFramesInFlight + 1;
        VkSemaphoreWaitInfo waitInfo  = {
            .sType          = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
            .semaphoreCount = 1,
            .pSemaphores    = &sync->graphicsTimeline,
            .pValues        = &waitValue,
        };
        VkResult waitResult = vkWaitSemaphores(device, &waitInfo, 100000000ULL); // 100ms
        if (waitResult == VK_TIMEOUT)
            return;
    }

#ifdef DEBUG
    MARK_TIME(tAfterSemaphore);

    // Read back GPU timestamps from the completed frame slot
    rgCollectGpuTimings(rg, dm->currentFrame);
#endif

    // GPU finished with this frame slot's commands.
    // Safe to destroy any resources enqueued MAX_FRAMES_IN_FLIGHT frames ago.
    flushDeletionQueue(&ld->deletionQueue, ld->device, ld->allocator);
    advanceDeletionQueue(&ld->deletionQueue);

#ifdef DEBUG
    MARK_TIME(tAfterDeletion);
#endif

    updateSSBODescriptorSets(ld->device, particleGraphics, dm->currentFrame, particleGraphics->maxFrames);

#ifdef DEBUG
    MARK_TIME(tAfterSSBO);
#endif

    uint32_t aquireImageIndex = dm->currentFrame;

    VkResult nextImageResult =
        vkAcquireNextImageKHR(device, dm->graphicPipeline->swapchain->VkSwapchain, 100000000ULL, // 100ms
                              gdp->imgViewSemaphore[dm->currentFrame], VK_NULL_HANDLE, &aquireImageIndex);

    switch (nextImageResult)
    {
    case VK_SUCCESS:
        break;
    case VK_SUBOPTIMAL_KHR:
    case VK_ERROR_OUT_OF_DATE_KHR:
        recreateSwapChain(pd, ld, window, rg);
        dm->currentFrame = (dm->currentFrame + 1) % gdp->maxFrames;
        return;
    case VK_TIMEOUT:
    case VK_NOT_READY:
        return;
    default:
        return; // Surface lost or other error — let the main loop handle shutdown
    }

#ifdef DEBUG
    MARK_TIME(tAfterAcquire);
#endif

    // --- Command recording ---
    VkCommandBuffer graphicsCB = gdp->commandBuffer[dm->currentFrame];
    VkCommandBuffer computeCB  = particleGraphics->computeP->commandBuffer[dm->currentFrame];

    VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };

    vkBeginCommandBuffer(graphicsCB, &beginInfo);
    vkBeginCommandBuffer(computeCB, &beginInfo);

    // Update per-frame resource handles
    rg->resources[0].image     = gdp->swapchain->p_scImages[dm->currentFrame];
    rg->resources[0].imageView = gdp->swapchain->p_scImageView[dm->currentFrame];

    uint32_t prevFrame = (dm->currentFrame - 1 + particleGraphics->maxFrames) % particleGraphics->maxFrames;
    rg->resources[4].buffer = particleGraphics->computeP->deviceBuffer->ssbos->buffers[prevFrame];
    rg->resources[5].buffer = particleGraphics->computeP->deviceBuffer->ssbos->buffers[dm->currentFrame];

    rg->deltaTime = deltaTime;

    rgExecute(rg, graphicsCB, computeCB, dm->currentFrame);

    // Transition swapchain image to present layout (on graphics CB)
    transitionImageLayout(NULL, VK_NULL_HANDLE,
                          gdp->swapchain->p_scImages[dm->currentFrame],
                          gdp->swapchain->currentSurfaceFormat->format,
                          VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                          VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, 1,
                          graphicsCB);

    vkEndCommandBuffer(graphicsCB);
    vkEndCommandBuffer(computeCB);

#ifdef DEBUG
    MARK_TIME(tAfterCmdRecord);
#endif

    // Resolve transform hierarchy before UBO fills — single forward pass
    resolveTransforms(&dm->graphicPipeline[0].scene->transformManager);

#ifdef DEBUG
    MARK_TIME(tAfterResolve);
#endif

    // Update per-frame globals (view, proj, invViewProj, camera) — computed once, written to binding 9
    FrameGlobals globals;
    updateFrameGlobals(&globals, dm->graphicPipeline->swapchain, camera);
    memcpy(dm->graphicPipeline[0].pGlobalsUbo->uniformBuffersMapped[dm->currentFrame], &globals,
           sizeof(FrameGlobals));

#ifdef DEBUG
    MARK_TIME(tAfterGlobals);
#endif

    // Model UBO fill — writes ModelInstanceUBO (mat4 model only) directly to mapped memory
    updateModelUniformBuffersDirect(gdp->deviceBuffer->pUbo->uniformBuffersMapped[dm->currentFrame],
                                    dm->graphicPipeline[0].scene);

#ifdef DEBUG
    MARK_TIME(tAfterModels);
#endif

    // Light UBO fill — writes LightInstanceUBO directly to mapped memory
    updateLightUniformBuffersDirect(lightGraphics->deviceBuffer->pUbo->uniformBuffersMapped[dm->currentFrame],
                                    deltaTime, dm->graphicPipeline[2].scene);

#ifdef DEBUG
    MARK_TIME(tAfterLights);
#endif

    // Submit both queues with timeline semaphore sync
    rgSubmit(rg, graphicsCB, computeCB, sync,
             gdp->imgViewSemaphore[dm->currentFrame],
             gdp->renderEndFlag[dm->currentFrame]);

#ifdef DEBUG
    MARK_TIME(tAfterSubmit);
#endif

    // Present
    VkSemaphore    presentWait[] = {gdp->renderEndFlag[dm->currentFrame]};
    VkSwapchainKHR swapChains[]  = {dm->graphicPipeline->swapchain->VkSwapchain};

    VkPresentInfoKHR presentInfo = {
        .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores    = presentWait,
        .swapchainCount     = 1,
        .pSwapchains        = swapChains,
        .pImageIndices      = &aquireImageIndex,
        .pResults           = NULL,
    };

    VkResult presentResult = vkQueuePresentKHR(*ld->queueManager.presentQueue, &presentInfo);

    if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR ||
        presentResult == VK_ERROR_SURFACE_LOST_KHR)
    {
        recreateSwapChain(pd, ld, window, rg);
    }

#ifdef DEBUG
    MARK_TIME(tAfterPresent);

    FrameTiming timing = {
        .semaphoreWait    = timespecDiffUs(&tFrameStart, &tAfterSemaphore),
        .deletionQueue    = timespecDiffUs(&tAfterSemaphore, &tAfterDeletion),
        .ssboDescriptor   = timespecDiffUs(&tAfterDeletion, &tAfterSSBO),
        .acquireImage     = timespecDiffUs(&tAfterSSBO, &tAfterAcquire),
        .cmdRecord        = timespecDiffUs(&tAfterAcquire, &tAfterCmdRecord),
        .resolveTransforms = timespecDiffUs(&tAfterCmdRecord, &tAfterResolve),
        .uboGlobals       = timespecDiffUs(&tAfterResolve, &tAfterGlobals),
        .uboModels        = timespecDiffUs(&tAfterGlobals, &tAfterModels),
        .uboLights        = timespecDiffUs(&tAfterModels, &tAfterLights),
        .submit           = timespecDiffUs(&tAfterLights, &tAfterSubmit),
        .present          = timespecDiffUs(&tAfterSubmit, &tAfterPresent),
        .totalCpu         = timespecDiffUs(&tFrameStart, &tAfterPresent),
    };
    accumulateTiming(&timing);
#endif

    dm->currentFrame = (dm->currentFrame + 1) % gdp->maxFrames;
}
