#include "camera.h"
#include "computePipeline.h"
#include "deletionQueue.h"
#include "displayManager.h"
#include "frustum.h"
#include "timelineSync.h"
#include "particle.h"
#include "renderGraph.h"
#include "scene.h"
#include "model.h"
#include "transformNode.h"
#include "transformations.h"
#include "mat4.h"
#include "affine.h"
#include "graphicConfiguration.h"
#include <assert.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

// ── Frame timing instrumentation ──────────────────────────────────────────────
#ifdef DEBUG
#define FRAME_TIMING_INTERVAL 3 // print averages every N frames

typedef struct FrameTiming
{
    double semaphoreWait;
    double deletionQueue;
    double ssboDescriptor;
    double acquireImage;
    double cmdRecord;
    double resolveTransforms;
    double uboGlobals;
    double frustumCull;
    double uboModels;
    double uboLights;
    double submit;
    double present;
    double totalCpu;
} FrameTiming;

static FrameTiming g_accumTiming  = {0};
static uint32_t    g_timingFrames = 0;

// Per-scenario sample buffers for min/median/p95 stats. Capacity is
// MAX_FRAME_COUNT which bounds the longest scenario. Entries written in
// arrival order; flushed + reset at scenario boundaries via
// displayBenchBeginScenario / displayBenchEndScenario.
static const char* g_benchScenarioName           = NULL;
static uint32_t    g_benchSampleCount            = 0;
static double      g_benchResolveSamples[MAX_FRAME_COUNT];
static double      g_benchCullSamples[MAX_FRAME_COUNT];
static double      g_benchTotalCpuSamples[MAX_FRAME_COUNT];
static double      g_benchGpuWaitSamples[MAX_FRAME_COUNT];   // vkWaitSemaphores on graphics timeline
static double      g_benchPresentSamples[MAX_FRAME_COUNT];   // vkQueuePresentKHR (FIFO blocks here)
static double      g_benchAcquireSamples[MAX_FRAME_COUNT];   // vkAcquireNextImageKHR

// Per-scenario LOD distribution accumulator. Each frame, displayBenchRecordLodSample
// walks the models and sums lodLiveInstanceCount[k] into g_benchLodBuckets[k].
// Reported as percentages in the scenario stats block.
static uint64_t g_benchLodBuckets[MODEL_LOD_COUNT];
static uint64_t g_benchLodTotal;
static uint32_t g_benchLodFrames;

static void benchPrintLodDistribution(void)
{
    if (g_benchLodTotal == 0 || g_benchLodFrames == 0)
    {
        printf("  %-18s (no samples)\n", "lod distribution");
        return;
    }
    double total = (double) g_benchLodTotal;
    printf("  %-18s", "lod distribution");
    for (int k = 0; k < MODEL_LOD_COUNT; k++)
    {
        double pct = 100.0 * (double) g_benchLodBuckets[k] / total;
        printf("  LOD%d=%7llu (%5.1f%%)", k, (unsigned long long) g_benchLodBuckets[k], pct);
    }
    printf("   avg/frame=%.0f\n", total / (double) g_benchLodFrames);
}

static inline double timespecDiffUs(struct timespec* start, struct timespec* end)
{
    return (double)(end->tv_sec - start->tv_sec) * 1e6 + (double)(end->tv_nsec - start->tv_nsec) / 1e3;
}

static int benchSampleCompare(const void* a, const void* b)
{
    double da = *(const double*)a;
    double db = *(const double*)b;
    return (da > db) - (da < db);
}

// Compute min/mean/median/p95/max over a sample buffer and print one line.
// Sorts a scratch copy so samples[] is untouched. Warmup transients are
// skipped by dropping the first 3 frames before stat computation — they
// correspond to the camera-change GPU pipeline re-warm, not steady state.
static void benchPrintStatsLine(const char* label, const double* samples, uint32_t count)
{
    const uint32_t warmupSkip = 3;
    if (count <= warmupSkip)
    {
        printf("  %-18s (n=%u too small)\n", label, count);
        return;
    }

    uint32_t n = count - warmupSkip;
    double*  scratch = (double*) malloc(n * sizeof(double));
    if (!scratch) return;
    for (uint32_t i = 0; i < n; i++) scratch[i] = samples[warmupSkip + i];

    qsort(scratch, n, sizeof(double), benchSampleCompare);

    double sum = 0.0;
    for (uint32_t i = 0; i < n; i++) sum += scratch[i];
    double mean   = sum / (double) n;
    double minV   = scratch[0];
    double maxV   = scratch[n - 1];
    double median = (n & 1u) ? scratch[n / 2] : 0.5 * (scratch[n / 2 - 1] + scratch[n / 2]);
    // p95 by nearest-rank — index = ceil(0.95 * n) - 1
    uint32_t p95idx = (uint32_t)((0.95 * (double) n) + 0.9999);
    if (p95idx >= n) p95idx = n - 1;
    else if (p95idx > 0) p95idx -= 1;
    double p95 = scratch[p95idx];

    printf("  %-18s min=%8.1f median=%8.1f mean=%8.1f p95=%8.1f max=%8.1f  (n=%u)\n",
           label, minV, median, mean, p95, maxV, n);

    free(scratch);
}

void displayBenchBeginScenario(const char* name)
{
    // Flush previous scenario's stats first, if any.
    if (g_benchScenarioName != NULL && g_benchSampleCount > 0)
    {
        printf("\n── Bench stats: '%s' (µs, warmup dropped) ──\n", g_benchScenarioName);
        benchPrintStatsLine("resolveTransforms", g_benchResolveSamples,  g_benchSampleCount);
        benchPrintStatsLine("frustumCull",       g_benchCullSamples,     g_benchSampleCount);
        benchPrintStatsLine("gpuFenceWait",      g_benchGpuWaitSamples,  g_benchSampleCount);
        benchPrintStatsLine("acquireImage",      g_benchAcquireSamples,  g_benchSampleCount);
        benchPrintStatsLine("present",           g_benchPresentSamples,  g_benchSampleCount);
        benchPrintStatsLine("totalCpu",          g_benchTotalCpuSamples, g_benchSampleCount);
        benchPrintLodDistribution();
        printf("──────────────────────────────────────────\n\n");
        fflush(stdout);
    }
    g_benchScenarioName = name;
    g_benchSampleCount  = 0;
    for (int k = 0; k < MODEL_LOD_COUNT; k++) g_benchLodBuckets[k] = 0;
    g_benchLodTotal  = 0;
    g_benchLodFrames = 0;
}

void displayBenchEndScenario(void)
{
    if (g_benchScenarioName != NULL && g_benchSampleCount > 0)
    {
        printf("\n── Bench stats: '%s' (µs, warmup dropped) ──\n", g_benchScenarioName);
        benchPrintStatsLine("resolveTransforms", g_benchResolveSamples,  g_benchSampleCount);
        benchPrintStatsLine("frustumCull",       g_benchCullSamples,     g_benchSampleCount);
        benchPrintStatsLine("gpuFenceWait",      g_benchGpuWaitSamples,  g_benchSampleCount);
        benchPrintStatsLine("acquireImage",      g_benchAcquireSamples,  g_benchSampleCount);
        benchPrintStatsLine("present",           g_benchPresentSamples,  g_benchSampleCount);
        benchPrintStatsLine("totalCpu",          g_benchTotalCpuSamples, g_benchSampleCount);
        benchPrintLodDistribution();
        printf("──────────────────────────────────────────\n\n");
        fflush(stdout);
    }
    g_benchScenarioName = NULL;
    g_benchSampleCount  = 0;
    for (int k = 0; k < MODEL_LOD_COUNT; k++) g_benchLodBuckets[k] = 0;
    g_benchLodTotal  = 0;
    g_benchLodFrames = 0;
}

void displayBenchRecordLodSample(const struct SCENE* scene)
{
    if (g_benchScenarioName == NULL || scene == NULL) return;
    for (uint32_t i = 0; i < scene->modelsManager.modelLen; i++)
    {
        const Model3D* m = &scene->modelsManager.pModel[i];
        if (m->alive != 1) continue;
        for (int k = 0; k < MODEL_LOD_COUNT; k++)
        {
            g_benchLodBuckets[k] += m->lodLiveInstanceCount[k];
            g_benchLodTotal      += m->lodLiveInstanceCount[k];
        }
    }
    g_benchLodFrames++;
}

static void accumulateTiming(FrameTiming* t)
{
    // Record raw per-frame samples into the active scenario buffer. Bounded
    // by MAX_FRAME_COUNT; extra samples past capacity are silently dropped
    // (scenario frame budgets sum to MAX_FRAME_COUNT so this only trips if
    // a caller forgets to call displayBenchBeginScenario).
    if (g_benchScenarioName != NULL && g_benchSampleCount < MAX_FRAME_COUNT)
    {
        g_benchResolveSamples[g_benchSampleCount]  = t->resolveTransforms;
        g_benchCullSamples[g_benchSampleCount]     = t->frustumCull;
        g_benchTotalCpuSamples[g_benchSampleCount] = t->totalCpu;
        g_benchGpuWaitSamples[g_benchSampleCount]  = t->semaphoreWait;
        g_benchAcquireSamples[g_benchSampleCount]  = t->acquireImage;
        g_benchPresentSamples[g_benchSampleCount]  = t->present;
        g_benchSampleCount++;
    }

    g_accumTiming.semaphoreWait    += t->semaphoreWait;
    g_accumTiming.deletionQueue    += t->deletionQueue;
    g_accumTiming.ssboDescriptor   += t->ssboDescriptor;
    g_accumTiming.acquireImage     += t->acquireImage;
    g_accumTiming.cmdRecord        += t->cmdRecord;
    g_accumTiming.resolveTransforms += t->resolveTransforms;
    g_accumTiming.uboGlobals       += t->uboGlobals;
    g_accumTiming.frustumCull      += t->frustumCull;
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
        printf("  frustum cull     : %8.1f\n", g_accumTiming.frustumCull / n);
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
#else // !DEBUG — provide no-op bench stubs so main.cpp can call them unconditionally.
void displayBenchBeginScenario(const char* name) { (void) name; }
void displayBenchEndScenario(void) {}
void displayBenchRecordLodSample(const struct SCENE* scene) { (void) scene; }
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

    // updateSSBODescriptorSets(ld->device, particleGraphics, dm->currentFrame, particleGraphics->maxFrames); // DORMANT A11 — bindless compute uses push-constant slot indices

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

    // Resolve transform hierarchy before any UBO/SSBO fills
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

    // Frustum cull — marks TRANSFORM_PASS_DISABLED on instances outside the view frustum
    FrustumPlanes frustum;
    frustumExtractPlanes(globals.viewProj, &frustum);
    frustumCullScene(dm->graphicPipeline[0].scene, &frustum);

#ifdef DEBUG
    MARK_TIME(tAfterFrustum);
#endif

    // Model UBO fill — writes ModelInstanceUBO (mat4 model only) directly to mapped memory.
    // The LOD classifier reads from scene->camera (eye + screenSize), which is never
    // written to elsewhere in the C engine — the live camera state lives in the C++
    // Scene wrapper. Sync it each frame so LOD pixel-size selection uses the real camera.
    dm->graphicPipeline[0].scene->camera = camera;
    updateModelUniformBuffersDirect(gdp->deviceBuffer->pUbo->uniformBuffersMapped[dm->currentFrame],
                                    dm->graphicPipeline[0].scene);

#ifdef DEBUG
    MARK_TIME(tAfterModels);
    // Record the per-frame LOD bucket distribution for the active scenario.
    displayBenchRecordLodSample(dm->graphicPipeline[0].scene);
#endif

    // Light SSBO fill — write to host-visible staging buffer
    updateLightUniformBuffersDirect(lightGraphics->deviceBuffer->pUbo->uniformBuffersMapped[dm->currentFrame],
                                    deltaTime, dm->graphicPipeline[2].scene);

#ifdef DEBUG
    MARK_TIME(tAfterLights);
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

    // Copy staging → device-local SSBO for model instances
    {
        UniformBufferManager* modelUbo  = gdp->deviceBuffer->pUbo;
        VkBufferCopy          copyRegion = {.srcOffset = 0, .dstOffset = 0, .size = modelUbo->stagingSize};
        vkCmdCopyBuffer(graphicsCB, modelUbo->stagingBuffers[dm->currentFrame],
                        modelUbo->uniformBuffers[dm->currentFrame], 1, &copyRegion);

        VkBufferMemoryBarrier barrier = {
            .sType               = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
            .srcAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT,
            .dstAccessMask       = VK_ACCESS_SHADER_READ_BIT,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .buffer              = modelUbo->uniformBuffers[dm->currentFrame],
            .offset              = 0,
            .size                = modelUbo->stagingSize,
        };
        vkCmdPipelineBarrier(graphicsCB, VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, 0, 0, NULL, 1,
                             &barrier, 0, NULL);
    }

    // Copy staging → device-local SSBO for light instances
    {
        UniformBufferManager* lightUbo = lightGraphics->deviceBuffer->pUbo;
        VkBufferCopy          copyRegion = {.srcOffset = 0, .dstOffset = 0, .size = lightUbo->stagingSize};
        vkCmdCopyBuffer(graphicsCB, lightUbo->stagingBuffers[dm->currentFrame],
                        lightUbo->uniformBuffers[dm->currentFrame], 1, &copyRegion);

        VkBufferMemoryBarrier barrier = {
            .sType               = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
            .srcAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT,
            .dstAccessMask       = VK_ACCESS_SHADER_READ_BIT,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .buffer              = lightUbo->uniformBuffers[dm->currentFrame],
            .offset              = 0,
            .size                = lightUbo->stagingSize,
        };
        vkCmdPipelineBarrier(graphicsCB, VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 1,
                             &barrier, 0, NULL);
    }

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
        .resolveTransforms = timespecDiffUs(&tAfterAcquire, &tAfterResolve),
        .uboGlobals       = timespecDiffUs(&tAfterResolve, &tAfterGlobals),
        .frustumCull      = timespecDiffUs(&tAfterGlobals, &tAfterFrustum),
        .uboModels        = timespecDiffUs(&tAfterFrustum, &tAfterModels),
        .uboLights        = timespecDiffUs(&tAfterModels, &tAfterLights),
        .cmdRecord        = timespecDiffUs(&tAfterLights, &tAfterCmdRecord),
        .submit           = timespecDiffUs(&tAfterCmdRecord, &tAfterSubmit),
        .present          = timespecDiffUs(&tAfterSubmit, &tAfterPresent),
        .totalCpu         = timespecDiffUs(&tFrameStart, &tAfterPresent),
    };
    accumulateTiming(&timing);
#endif

    dm->currentFrame = (dm->currentFrame + 1) % gdp->maxFrames;
}
