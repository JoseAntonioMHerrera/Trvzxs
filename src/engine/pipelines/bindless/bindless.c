#include "bindless.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void queryAndClampLimits(VkPhysicalDevice gpu, uint32_t* maxTextures, uint32_t* maxBuffers)
{
    VkPhysicalDeviceDescriptorIndexingProperties idxProps = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_PROPERTIES,
    };
    VkPhysicalDeviceProperties2 props2 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
        .pNext = &idxProps,
    };
    vkGetPhysicalDeviceProperties2(gpu, &props2);

    // Clamp caller caps against UpdateAfterBind per-stage + per-descriptor limits.
    const uint32_t texLimit = idxProps.maxDescriptorSetUpdateAfterBindSampledImages;
    const uint32_t bufLimit = idxProps.maxDescriptorSetUpdateAfterBindStorageBuffers;
    if (*maxTextures > texLimit) *maxTextures = texLimit;
    if (*maxBuffers  > bufLimit) *maxBuffers  = bufLimit;
}

void bindlessInit(BindlessSystem* bs, VkDevice device, VkPhysicalDevice gpu,
                  uint32_t maxTextures, uint32_t maxBuffers)
{
    assert(bs && device && gpu);
    queryAndClampLimits(gpu, &maxTextures, &maxBuffers);

    bs->device               = device;
    bs->maxTextures          = maxTextures;
    bs->maxBuffers           = maxBuffers;
    bs->textureCount         = 0;
    bs->bufferCount          = 0;
    bs->textureFreeList      = calloc(maxTextures, sizeof(uint32_t));
    bs->bufferFreeList       = calloc(maxBuffers,  sizeof(uint32_t));
    bs->textureFreeListCount = 0;
    bs->bufferFreeListCount  = 0;

    const VkDescriptorSetLayoutBinding bindings[2] = {
        {
            .binding         = BINDLESS_BINDING_TEXTURES,
            .descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = maxTextures,
            .stageFlags      = VK_SHADER_STAGE_ALL,
        },
        {
            .binding         = BINDLESS_BINDING_BUFFERS,
            .descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = maxBuffers,
            .stageFlags      = VK_SHADER_STAGE_ALL,
        },
    };

    const VkDescriptorBindingFlags flags =
        VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT |
        VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT |
        VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT;
    const VkDescriptorBindingFlags bindingFlags[2] = {flags, flags};

    const VkDescriptorSetLayoutBindingFlagsCreateInfo flagsInfo = {
        .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
        .bindingCount  = 2,
        .pBindingFlags = bindingFlags,
    };

    const VkDescriptorSetLayoutCreateInfo layoutInfo = {
        .sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext        = &flagsInfo,
        .flags        = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT,
        .bindingCount = 2,
        .pBindings    = bindings,
    };
    if (vkCreateDescriptorSetLayout(device, &layoutInfo, NULL, &bs->globalLayout) != VK_SUCCESS)
    {
        fprintf(stderr, "bindlessInit: vkCreateDescriptorSetLayout failed\n");
        exit(1);
    }

    const VkDescriptorPoolSize poolSizes[2] = {
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, maxTextures},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,         maxBuffers},
    };
    const VkDescriptorPoolCreateInfo poolInfo = {
        .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .flags         = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT,
        .maxSets       = 1,
        .poolSizeCount = 2,
        .pPoolSizes    = poolSizes,
    };
    if (vkCreateDescriptorPool(device, &poolInfo, NULL, &bs->globalPool) != VK_SUCCESS)
    {
        fprintf(stderr, "bindlessInit: vkCreateDescriptorPool failed\n");
        exit(1);
    }

    const VkDescriptorSetAllocateInfo allocInfo = {
        .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool     = bs->globalPool,
        .descriptorSetCount = 1,
        .pSetLayouts        = &bs->globalLayout,
    };
    if (vkAllocateDescriptorSets(device, &allocInfo, &bs->globalSet) != VK_SUCCESS)
    {
        fprintf(stderr, "bindlessInit: vkAllocateDescriptorSets failed\n");
        exit(1);
    }
}

void bindlessDestroy(BindlessSystem* bs)
{
    if (!bs || !bs->device) return;
    if (bs->globalPool)   vkDestroyDescriptorPool(bs->device, bs->globalPool, NULL);
    if (bs->globalLayout) vkDestroyDescriptorSetLayout(bs->device, bs->globalLayout, NULL);
    free(bs->textureFreeList);
    free(bs->bufferFreeList);
    memset(bs, 0, sizeof(*bs));
}

static uint32_t allocSlot(uint32_t* freeList, uint32_t* freeCount, uint32_t* count, uint32_t cap)
{
    if (*freeCount > 0) return freeList[--(*freeCount)];
    assert(*count < cap && "bindless slot capacity exhausted");
    return (*count)++;
}

BindlessHandle bindlessRegisterTexture(BindlessSystem* bs, VkImageView view, VkSampler sampler)
{
    const uint32_t index = allocSlot(bs->textureFreeList, &bs->textureFreeListCount,
                                     &bs->textureCount, bs->maxTextures);
    const VkDescriptorImageInfo img = {
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        .imageView   = view,
        .sampler     = sampler,
    };
    const VkWriteDescriptorSet write = {
        .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet          = bs->globalSet,
        .dstBinding      = BINDLESS_BINDING_TEXTURES,
        .dstArrayElement = index,
        .descriptorCount = 1,
        .descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .pImageInfo      = &img,
    };
    vkUpdateDescriptorSets(bs->device, 1, &write, 0, NULL);
    return index;
}

void bindlessUnregisterTexture(BindlessSystem* bs, BindlessHandle h)
{
    if (h == BINDLESS_INVALID) return;
    assert(h < bs->maxTextures);
    bs->textureFreeList[bs->textureFreeListCount++] = h;
    // Slot stays PARTIALLY_BOUND — no descriptor write needed. Caller must
    // ensure no in-flight command still references it.
}

BindlessHandle bindlessRegisterBuffer(BindlessSystem* bs, VkBuffer buffer, VkDeviceSize size)
{
    const uint32_t index = allocSlot(bs->bufferFreeList, &bs->bufferFreeListCount,
                                     &bs->bufferCount, bs->maxBuffers);
    const VkDescriptorBufferInfo buf = {
        .buffer = buffer,
        .offset = 0,
        .range  = size,
    };
    const VkWriteDescriptorSet write = {
        .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet          = bs->globalSet,
        .dstBinding      = BINDLESS_BINDING_BUFFERS,
        .dstArrayElement = index,
        .descriptorCount = 1,
        .descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .pBufferInfo     = &buf,
    };
    vkUpdateDescriptorSets(bs->device, 1, &write, 0, NULL);
    return index;
}

void bindlessUnregisterBuffer(BindlessSystem* bs, BindlessHandle h)
{
    if (h == BINDLESS_INVALID) return;
    assert(h < bs->maxBuffers);
    bs->bufferFreeList[bs->bufferFreeListCount++] = h;
}

void bindlessBindGlobal(BindlessSystem* bs, VkCommandBuffer cmd,
                        VkPipelineLayout layout, VkPipelineBindPoint bindPoint)
{
    vkCmdBindDescriptorSets(cmd, bindPoint, layout,
                            BINDLESS_SET_INDEX, 1, &bs->globalSet, 0, NULL);
}

void bindlessWriteLightPassGBufferSamplers(VkDevice device, uint32_t maxFrames,
                                           VkDescriptorSet* lightPerPassSets,
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
                .dstSet          = lightPerPassSets[i],
                .dstBinding      = 1,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .pImageInfo      = &gImg,
            },
            {
                .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet          = lightPerPassSets[i],
                .dstBinding      = 2,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .pImageInfo      = &nImg,
            },
        };
        vkUpdateDescriptorSets(device, 2, writes, 0, NULL);
    }
}
