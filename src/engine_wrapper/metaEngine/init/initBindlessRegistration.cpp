#include "initBindlessRegistration.hpp"

#include <cstdio>
#include <cstdlib>

extern "C"
{
#include "bindless.h"
#include "perPassLayouts.h"
#include "transformations.h"
#include "particle.h"
#include "bufferTypes.h"
#include "clusterConfig.h"
#include "clusterGrid.h"
#include "lightCullResources.h"
}

static void allocPerPassPoolAndSets(VkDevice device,
                                    VkDescriptorSetLayout layout,
                                    uint32_t maxFrames,
                                    VkDescriptorPool *poolOut,
                                    VkDescriptorSet **setsOut)
{
    const VkDescriptorPoolSize poolSize = {
        .type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = maxFrames,
    };
    const VkDescriptorPoolCreateInfo poolInfo = {
        .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets       = maxFrames,
        .poolSizeCount = 1,
        .pPoolSizes    = &poolSize,
    };
    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, poolOut) != VK_SUCCESS)
    {
        std::fprintf(stderr, "initBindlessRegistration: perPass pool creation failed\n");
        std::exit(1);
    }

    VkDescriptorSetLayout *layoutPerFrame = (VkDescriptorSetLayout *)malloc(sizeof(VkDescriptorSetLayout) * maxFrames);
    for (uint32_t i = 0; i < maxFrames; ++i) layoutPerFrame[i] = layout;

    const VkDescriptorSetAllocateInfo allocInfo = {
        .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool     = *poolOut,
        .descriptorSetCount = maxFrames,
        .pSetLayouts        = layoutPerFrame,
    };
    *setsOut = (VkDescriptorSet *)malloc(sizeof(VkDescriptorSet) * maxFrames);
    if (vkAllocateDescriptorSets(device, &allocInfo, *setsOut) != VK_SUCCESS)
    {
        std::fprintf(stderr, "initBindlessRegistration: perPass set allocation failed\n");
        std::exit(1);
    }
    free(layoutPerFrame);
}

// Writes FrameGlobals UBO (binding 0) into each per-frame per-pass set.
static void writeFrameGlobalsUBO(VkDevice device, uint32_t maxFrames,
                                 VkDescriptorSet *sets,
                                 UniformBufferManager *frameGlobals)
{
    for (uint32_t i = 0; i < maxFrames; ++i)
    {
        const VkDescriptorBufferInfo buf = {
            .buffer = frameGlobals->uniformBuffers[i],
            .offset = 0,
            // FrameGlobals UBO is created by createFrameGlobalsUniformBuffersMapped
            // which does not populate stagingSize (it's a mapped UBO, not
            // staging+device). Use sizeof(FrameGlobals) explicitly.
            .range  = sizeof(FrameGlobals),
        };
        const VkWriteDescriptorSet write = {
            .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet          = sets[i],
            .dstBinding      = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .pBufferInfo     = &buf,
        };
        vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
    }
}

// Writes G-buffer + normals texture samplers at bindings 1, 2 for every
// per-frame per-pass set (light pass). Both views share the same sampler.
static void writeLightPassSamplers(VkDevice device, uint32_t maxFrames,
                                   VkDescriptorSet *sets,
                                   VkImageView gBufferView,
                                   VkImageView normalsView,
                                   VkSampler sampler)
{
    for (uint32_t i = 0; i < maxFrames; ++i)
    {
        const VkDescriptorImageInfo gImg = {
            .sampler     = sampler,
            .imageView   = gBufferView,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };
        const VkDescriptorImageInfo nImg = {
            .sampler     = sampler,
            .imageView   = normalsView,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };
        const VkWriteDescriptorSet writes[2] = {
            {
                .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet          = sets[i],
                .dstBinding      = 1,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .pImageInfo      = &gImg,
            },
            {
                .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet          = sets[i],
                .dstBinding      = 2,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .pImageInfo      = &nImg,
            },
        };
        vkUpdateDescriptorSets(device, 2, writes, 0, nullptr);
    }
}

// Light-pass variant of the per-pass pool: one UBO (frame globals) +
// two COMBINED_IMAGE_SAMPLER (G-buffer + normals) per allocated set.
static void allocLightPerPassPoolAndSets(VkDevice device,
                                         VkDescriptorSetLayout layout,
                                         uint32_t maxFrames,
                                         VkDescriptorPool *poolOut,
                                         VkDescriptorSet **setsOut)
{
    const VkDescriptorPoolSize poolSizes[2] = {
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         maxFrames},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, maxFrames * 2},
    };
    const VkDescriptorPoolCreateInfo poolInfo = {
        .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets       = maxFrames,
        .poolSizeCount = 2,
        .pPoolSizes    = poolSizes,
    };
    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, poolOut) != VK_SUCCESS)
    {
        std::fprintf(stderr, "allocLightPerPassPoolAndSets: pool create failed\n");
        std::exit(1);
    }

    VkDescriptorSetLayout *layoutPerFrame = (VkDescriptorSetLayout *)malloc(sizeof(VkDescriptorSetLayout) * maxFrames);
    for (uint32_t i = 0; i < maxFrames; ++i) layoutPerFrame[i] = layout;

    const VkDescriptorSetAllocateInfo allocInfo = {
        .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool     = *poolOut,
        .descriptorSetCount = maxFrames,
        .pSetLayouts        = layoutPerFrame,
    };
    *setsOut = (VkDescriptorSet *)malloc(sizeof(VkDescriptorSet) * maxFrames);
    if (vkAllocateDescriptorSets(device, &allocInfo, *setsOut) != VK_SUCCESS)
    {
        std::fprintf(stderr, "allocLightPerPassPoolAndSets: set alloc failed\n");
        std::exit(1);
    }
    free(layoutPerFrame);
}

void metaInitBindlessRegistration(MetaInitContext &ctx)
{
    LogicalDevice            *ld           = ctx.ld;
    GraphicPipelineToSurface *gdp          = ctx.gdp;
    GraphicPipelineToSurface *depthGdp     = ctx.depthPrepassPipeline;
    GraphicPipelineToSurface *lightGdp     = ctx.lightPipeline;
    GraphicPipelineToSurface *particleGdp  = ctx.particlePipeline;

    BindlessSystem *bs = &ld->bindless;
    PerPassLayouts *pp = &ld->perPassLayouts;
    const uint32_t  maxFrames = gdp->maxFrames;

    // --- Register model texture into the global table -----------------------
    const BindlessHandle texHandle =
        bindlessRegisterTexture(bs, gdp->texture->common->imageView, gdp->texture->sampler);

    // --- Register each frame's transform SSBO -------------------------------
    // The transform array is double/triple buffered: one SSBO per frame-in-
    // flight. Each gets its own bindless slot. The draw recorder selects the
    // slot matching currentFrame and pushes it as transformIdx.
    UniformBufferManager *transformUbos = gdp->deviceBuffer->pUbo; // actually SSBO; legacy name
    BindlessHandle       *transformHandles = (BindlessHandle *)malloc(sizeof(BindlessHandle) * maxFrames);
    for (uint32_t i = 0; i < maxFrames; ++i)
    {
        transformHandles[i] = bindlessRegisterBuffer(bs, transformUbos->uniformBuffers[i],
                                                     transformUbos->stagingSize);
    }

    // --- Allocate + fill model pass set-1 descriptor sets -------------------
    VkDescriptorPool modelPool  = VK_NULL_HANDLE;
    VkDescriptorSet *modelSets  = nullptr;
    allocPerPassPoolAndSets(ld->device, pp->modelPass, maxFrames, &modelPool, &modelSets);
    writeFrameGlobalsUBO(ld->device, maxFrames, modelSets, gdp->pGlobalsUbo);

    // --- Allocate + fill depth prepass set-1 descriptor sets ----------------
    VkDescriptorPool depthPool = VK_NULL_HANDLE;
    VkDescriptorSet *depthSets = nullptr;
    allocPerPassPoolAndSets(ld->device, pp->depthPrepassPass, maxFrames, &depthPool, &depthSets);
    writeFrameGlobalsUBO(ld->device, maxFrames, depthSets, gdp->pGlobalsUbo);

    // --- Plant handles on both pipelines ------------------------------------
    gdp->bindlessGlobalLayout      = bs->globalLayout;
    gdp->bindlessGlobalSet         = bs->globalSet;
    gdp->bindlessPerPassLayout     = pp->modelPass;
    gdp->perPassPool               = modelPool;
    gdp->perPassSets               = modelSets;
    gdp->bindlessTextureHandle     = texHandle;
    gdp->bindlessTransformHandles  = transformHandles;

    depthGdp->bindlessGlobalLayout     = bs->globalLayout;
    depthGdp->bindlessGlobalSet        = bs->globalSet;
    depthGdp->bindlessPerPassLayout    = pp->depthPrepassPass;
    depthGdp->perPassPool              = depthPool;
    depthGdp->perPassSets              = depthSets;
    depthGdp->bindlessTextureHandle    = texHandle;            // unused in depth pass, kept for parity
    depthGdp->bindlessTransformHandles = transformHandles;     // share — same data

    // --- Light pipeline ------------------------------------------------------
    UniformBufferManager *lightUbos = lightGdp->deviceBuffer->pUbo;
    BindlessHandle       *lightTransformHandles =
        (BindlessHandle *)malloc(sizeof(BindlessHandle) * maxFrames);
    for (uint32_t i = 0; i < maxFrames; ++i)
    {
        lightTransformHandles[i] =
            bindlessRegisterBuffer(bs, lightUbos->uniformBuffers[i], lightUbos->stagingSize);
    }

    VkDescriptorPool lightPool = VK_NULL_HANDLE;
    VkDescriptorSet *lightSets = nullptr;
    allocLightPerPassPoolAndSets(ld->device, pp->lightPass, maxFrames, &lightPool, &lightSets);
    // Frame globals UBO is engine-shared — only gdp->pGlobalsUbo is populated
    // (initUniformBuffers.cpp). All pipelines reference the same buffer set.
    writeFrameGlobalsUBO(ld->device, maxFrames, lightSets, gdp->pGlobalsUbo);
    writeLightPassSamplers(ld->device, maxFrames, lightSets,
                           gdp->swapchain->gBuffer->imageView,
                           gdp->swapchain->nBuffer->imageView,
                           gdp->swapchain->gBufferSampler);

    lightGdp->bindlessGlobalLayout      = bs->globalLayout;
    lightGdp->bindlessGlobalSet         = bs->globalSet;
    lightGdp->bindlessPerPassLayout     = pp->lightPass;
    lightGdp->perPassPool               = lightPool;
    lightGdp->perPassSets               = lightSets;
    lightGdp->bindlessTextureHandle     = BINDLESS_INVALID;   // unused; frag reads per-pass textures
    lightGdp->bindlessTransformHandles  = lightTransformHandles;

    // --- Particle pipelines (compute + render) -----------------------------
    // Particle SSBOs are per-frame-in-flight (one buffer each). Each is
    // registered as its own bindless slot so compute can pick prev/current
    // via push constants and the render pass can read current.
    SSBOBufferManager *particleSsbos = particleGdp->computeP->deviceBuffer->ssbos;
    BindlessHandle    *particleSSBOHandles =
        (BindlessHandle *)malloc(sizeof(BindlessHandle) * maxFrames);
    const VkDeviceSize particleBufSize =
        sizeof(ParticleInstance) *
        particleGdp->scene->particleManager.packedParticleInstanceLen;
    for (uint32_t i = 0; i < maxFrames; ++i)
    {
        particleSSBOHandles[i] =
            bindlessRegisterBuffer(bs, particleSsbos->buffers[i], particleBufSize);
    }

    // Render pass per-pass set: binding 0 = FrameGlobals UBO. Reuse modelPass
    // layout (identical shape).
    VkDescriptorPool particleRenderPool = VK_NULL_HANDLE;
    VkDescriptorSet *particleRenderSets = nullptr;
    allocPerPassPoolAndSets(ld->device, pp->modelPass, maxFrames,
                            &particleRenderPool, &particleRenderSets);
    writeFrameGlobalsUBO(ld->device, maxFrames, particleRenderSets, gdp->pGlobalsUbo);

    // Compute pass per-pass set: binding 0 = deltaTime UBO. A single-frame
    // buffer lives on computeP->deviceBuffer->pUbo->uniformBuffers[0].
    VkDescriptorPool particleComputePool = VK_NULL_HANDLE;
    VkDescriptorSet *particleComputeSets = nullptr;
    allocPerPassPoolAndSets(ld->device, pp->particleComputePass, maxFrames,
                            &particleComputePool, &particleComputeSets);
    {
        UniformBufferManager *deltaUbo = particleGdp->computeP->deviceBuffer->pUbo;
        for (uint32_t i = 0; i < maxFrames; ++i)
        {
            const VkDescriptorBufferInfo buf = {
                .buffer = deltaUbo->uniformBuffers[0], // single buffer shared across frames
                .offset = 0,
                .range  = sizeof(float),
            };
            const VkWriteDescriptorSet write = {
                .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet          = particleComputeSets[i],
                .dstBinding      = 0,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .pBufferInfo     = &buf,
            };
            vkUpdateDescriptorSets(ld->device, 1, &write, 0, nullptr);
        }
    }

    // Plant on particle graphics pipeline (render path)
    particleGdp->bindlessGlobalLayout      = bs->globalLayout;
    particleGdp->bindlessGlobalSet         = bs->globalSet;
    particleGdp->bindlessPerPassLayout     = pp->modelPass;
    particleGdp->perPassPool               = particleRenderPool;
    particleGdp->perPassSets               = particleRenderSets;
    particleGdp->bindlessTextureHandle     = BINDLESS_INVALID;
    particleGdp->bindlessTransformHandles  = particleSSBOHandles;

    // Plant on compute pipeline (shared SSBO handle array via graphics gdp)
    ComputePipeline *cp = particleGdp->computeP;
    cp->bindlessGlobalLayout  = bs->globalLayout;
    cp->bindlessGlobalSet     = bs->globalSet;
    cp->bindlessPerPassLayout = pp->particleComputePass;
    cp->perPassPool           = particleComputePool;
    cp->perPassSets           = particleComputeSets;

    // --- Clustered forward+: cluster AABB SSBOs on the lightCull compute stub ---
    // Phase 1 only registers the AABB ring (input to the future lightCull.comp).
    // Phase 2 will register lightIndexList / lightGrid / globalIndexCount here
    // once those buffers exist. The lightCull compute pipeline is otherwise a
    // stub — no pipeline handle, no dispatch, no perPass layout yet.
    ComputePipeline       *lightCullCP = &lightGdp->computeP[0];
    SSBOBufferManager     *clusterSsbos = lightCullCP->deviceBuffer->ssbos;
    const VkDeviceSize     clusterBufSize = sizeof(ClusterAABB) * CLUSTER_COUNT;
    BindlessHandle        *clusterHandles =
        (BindlessHandle *)malloc(sizeof(BindlessHandle) * lightCullCP->maxFrames);
    for (uint32_t i = 0; i < lightCullCP->maxFrames; ++i)
    {
        clusterHandles[i] = bindlessRegisterBuffer(bs, clusterSsbos->buffers[i], clusterBufSize);
    }
    lightCullCP->bindlessClusterAABBHandles = clusterHandles;

    // Phase 2a: register the three lightCull output buffer rings. Each ring has
    // maxFrames copies; register each as its own bindless slot. The compute
    // pass selects the slot for the current frame via push constant / direct index.
    LightCullResources *lcr = (LightCullResources *)lightCullCP->lightCullResources;
    if (lcr != nullptr)
    {
        const VkDeviceSize indexListBufSize =
            (VkDeviceSize)CLUSTER_COUNT * MAX_LIGHTS_PER_CLUSTER * sizeof(uint32_t);
        const VkDeviceSize gridBufSize       = (VkDeviceSize)CLUSTER_COUNT * 2 * sizeof(uint32_t);
        const VkDeviceSize counterBufSize    = sizeof(uint32_t);

        lcr->lightIndexListHandles   =
            (BindlessHandle *)malloc(sizeof(BindlessHandle) * lightCullCP->maxFrames);
        lcr->lightGridHandles        =
            (BindlessHandle *)malloc(sizeof(BindlessHandle) * lightCullCP->maxFrames);
        lcr->globalIndexCountHandles =
            (BindlessHandle *)malloc(sizeof(BindlessHandle) * lightCullCP->maxFrames);

        for (uint32_t i = 0; i < lightCullCP->maxFrames; ++i)
        {
            lcr->lightIndexListHandles[i]   =
                bindlessRegisterBuffer(bs, lcr->lightIndexList.buffers[i], indexListBufSize);
            lcr->lightGridHandles[i]        =
                bindlessRegisterBuffer(bs, lcr->lightGrid.buffers[i], gridBufSize);
            lcr->globalIndexCountHandles[i] =
                bindlessRegisterBuffer(bs, lcr->globalIndexCount.buffers[i], counterBufSize);
        }
    }
}
