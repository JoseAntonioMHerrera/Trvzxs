#include "initBindlessRegistration.hpp"

#include <cstdio>
#include <cstdlib>

extern "C"
{
#include "bindless.h"
#include "perPassLayouts.h"
#include "transformations.h"
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

void metaInitBindlessRegistration(MetaInitContext &ctx)
{
    LogicalDevice            *ld      = ctx.ld;
    GraphicPipelineToSurface *gdp     = ctx.gdp;
    GraphicPipelineToSurface *depthGdp = ctx.depthPrepassPipeline;

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
}
