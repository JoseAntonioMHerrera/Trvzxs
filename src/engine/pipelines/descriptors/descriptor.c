
#include "descriptor.h"
#include "vertex.h"
#include <assert.h>
#include "particle.h"
#include "graphicConfiguration.h"

void createDescriptorPool(uint32_t maxFrames, VkDevice device, GraphicPipelineToSurface* gdp)
{
    VkDescriptorPoolSize       poolSizes[] = {{
                                                  .type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                                  .descriptorCount = maxFrames * 4, // models + lights + deltaTime + globals
                                        },
                                              {
                                                  .type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                                  .descriptorCount = maxFrames * 3, // texture + gBuffer + nBuffer
                                        },
                                              {
                                                  .type            = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                                  .descriptorCount = maxFrames * 3, // prevSSBO + curSSBO + readback
                                        }};
    VkDescriptorPoolCreateInfo poolInfo    = {
           .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
           .poolSizeCount = 3,
           .pPoolSizes    = poolSizes,
           .maxSets       = maxFrames,
           .flags         = 0,
           .pNext         = VK_NULL_HANDLE,
    };

    poolInfo.maxSets = maxFrames;
    // TODO moved descriptorPool to stack
    //  gdp->descriptorPool = malloc(sizeof(*gdp->descriptorPool));
    if (vkCreateDescriptorPool(device, &poolInfo, VK_NULL_HANDLE, &(gdp->descriptorPool)) != VK_SUCCESS)
    {
    }
}
// TODO must receive n textures that will be rendered in screen
void createDescriptorSet(uint32_t maxFrames, VkDevice device, GraphicPipelineToSurface* gdp,
                         GraphicPipelineToSurface* particleGraphic, GraphicPipelineToSurface* lightGraphic,
                         Texture2D* texture)
{
    assert(gdp->dsl);

    // We allocate one descriptorset for
    gdp->descriptorSet = malloc(sizeof(*gdp->descriptorSet) * maxFrames);

    VkDescriptorSetAllocateInfo allocInfo = {
        .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool     = gdp->descriptorPool,
        .descriptorSetCount = maxFrames,
        .pSetLayouts        = (VkDescriptorSetLayout[]) {gdp->dsl, gdp->dsl, gdp->dsl},
    };

    if (vkAllocateDescriptorSets(device, &allocInfo, gdp->descriptorSet) != VK_SUCCESS)
    {
    }

    for (size_t i = 0; i < maxFrames; i++)
    {
        VkDescriptorBufferInfo transformUBOInfo[2] = {
            {
                .buffer = gdp->deviceBuffer->pUbo->uniformBuffers[i],
                .offset = 0,
                .range  = sizeof(ModelInstanceUBO) * gdp->scene->modelsManager.modelInstanceCapacity,
            },
            {
                .buffer = lightGraphic->deviceBuffer->pUbo->uniformBuffers[i],
                .offset = 0,
                .range  = sizeof(LightInstanceUBO) * lightGraphic->scene->lightManager.lightInstanceCapacity,
            }};

        VkDescriptorBufferInfo deltaUBOInfo = {
            .buffer = gdp->deviceBuffer->pUbo->uniformBuffers[i],
            .offset = 0,
            .range  = sizeof(float),
        };

        VkDescriptorBufferInfo prevSSBOBufferInfo = {
            .buffer = particleGraphic->computeP->deviceBuffer->ssbos->buffers[(i - 1 + maxFrames) % maxFrames],
            .offset = 0,
            .range  = sizeof(ParticleInstance) * particleGraphic->scene->particleManager.packedParticleInstanceLen,
        };

        VkDescriptorBufferInfo currentSSBOBufferInfo = {
            .buffer = particleGraphic->computeP->deviceBuffer->ssbos->buffers[i],
            .offset = 0,
            .range  = sizeof(ParticleInstance) * particleGraphic->scene->particleManager.packedParticleInstanceLen,
        };

        VkDescriptorBufferInfo lightPixelReadBack = {
            .buffer = lightGraphic->deviceBuffer->ssbos->readbackBuffer,
            .offset = 0,
            .range  = sizeof(ReadBackBuffer) * WINDOW_WIDTH * WINDOW_HEIGHT,
        };
        VkDescriptorImageInfo imgInfo = {
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .imageView   = texture->common->imageView,
            .sampler     = texture->sampler,
        };

        VkDescriptorImageInfo gBuffer = {
            .sampler     = gdp->swapchain->gBufferSampler,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .imageView   = gdp->swapchain->gBuffer->imageView,
        };
        VkDescriptorImageInfo nBuffer = {
            .sampler     = gdp->swapchain->gBufferSampler,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .imageView   = gdp->swapchain->nBuffer->imageView,
        };
        VkDescriptorBufferInfo globalsUBOInfo = {
            .buffer = gdp->pGlobalsUbo->uniformBuffers[i],
            .offset = 0,
            .range  = sizeof(FrameGlobals),
        };

        VkWriteDescriptorSet descriptorWrite[] = {
            {
                .sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet           = gdp->descriptorSet[i],
                .dstBinding       = 0,
                .dstArrayElement  = 0,
                .descriptorType   = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .descriptorCount  = 1,
                .pBufferInfo      = &transformUBOInfo[0],
                .pImageInfo       = VK_NULL_HANDLE, // Optional
                .pTexelBufferView = VK_NULL_HANDLE, // Optional
            },
            {
                .sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet           = gdp->descriptorSet[i],
                .dstBinding       = 1,
                .dstArrayElement  = 0,
                .descriptorType   = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount  = 1,
                .pImageInfo       = &imgInfo,
                .pTexelBufferView = VK_NULL_HANDLE, // Optional
            },
            {
                .sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet           = gdp->descriptorSet[i],
                .dstBinding       = 2,
                .dstArrayElement  = 0,
                .descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .descriptorCount  = 1,
                .pBufferInfo      = &prevSSBOBufferInfo,
                .pTexelBufferView = VK_NULL_HANDLE, // Optional
            },
            {
                .sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet           = gdp->descriptorSet[i],
                .dstBinding       = 3,
                .dstArrayElement  = 0,
                .descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .descriptorCount  = 1,
                .pBufferInfo      = &currentSSBOBufferInfo,
                .pTexelBufferView = VK_NULL_HANDLE, // Optional
            },
            {
                .sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet           = gdp->descriptorSet[i],
                .dstBinding       = 4,
                .dstArrayElement  = 0,
                .descriptorType   = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .descriptorCount  = 1,
                .pBufferInfo      = &deltaUBOInfo,
                .pImageInfo       = VK_NULL_HANDLE, // Optional
                .pTexelBufferView = VK_NULL_HANDLE, // Optional
            },
            {
                .sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet           = gdp->descriptorSet[i],
                .dstBinding       = 5,
                .dstArrayElement  = 0,
                .descriptorType   = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount  = 1,
                .pBufferInfo      = VK_NULL_HANDLE,
                .pImageInfo       = &gBuffer,
                .pTexelBufferView = VK_NULL_HANDLE,
            },
            {
                .sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet           = gdp->descriptorSet[i],
                .dstBinding       = 6,
                .dstArrayElement  = 0,
                .descriptorType   = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount  = 1,
                .pBufferInfo      = VK_NULL_HANDLE,
                .pImageInfo       = &nBuffer,
                .pTexelBufferView = VK_NULL_HANDLE,
            },
            {
                .sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet           = gdp->descriptorSet[i],
                .dstBinding       = 7,
                .dstArrayElement  = 0,
                .descriptorType   = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .descriptorCount  = 1,
                .pBufferInfo      = &transformUBOInfo[1],
                .pImageInfo       = VK_NULL_HANDLE, // Optional
                .pTexelBufferView = VK_NULL_HANDLE, // Optional
            },
            {
                .sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet           = gdp->descriptorSet[i],
                .dstBinding       = 8,
                .dstArrayElement  = 0,
                .descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .descriptorCount  = 1,
                .pBufferInfo      = &lightPixelReadBack,
                .pImageInfo       = VK_NULL_HANDLE, // Optional
                .pTexelBufferView = VK_NULL_HANDLE, // Optional
            },                                      // TODO: add preprocessing for switching readbackbuffers debugging
            {
                .sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet           = gdp->descriptorSet[i],
                .dstBinding       = 9,
                .dstArrayElement  = 0,
                .descriptorType   = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .descriptorCount  = 1,
                .pBufferInfo      = &globalsUBOInfo,
                .pImageInfo       = VK_NULL_HANDLE,
                .pTexelBufferView = VK_NULL_HANDLE,
            },
        };
        vkUpdateDescriptorSets(device, 10, descriptorWrite, 0, VK_NULL_HANDLE);
    }
}

void updateGBufferDescriptors(uint32_t maxFrames, VkDevice device, GraphicPipelineToSurface* gdp)
{
    for (size_t i = 0; i < maxFrames; i++)
    {
        VkDescriptorImageInfo gBuffer = {
            .sampler     = gdp->swapchain->gBufferSampler,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .imageView   = gdp->swapchain->gBuffer->imageView,
        };
        VkDescriptorImageInfo nBuffer = {
            .sampler     = gdp->swapchain->gBufferSampler,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .imageView   = gdp->swapchain->nBuffer->imageView,
        };
        VkWriteDescriptorSet writes[] = {
            {
                .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet          = gdp->descriptorSet[i],
                .dstBinding      = 5,
                .dstArrayElement = 0,
                .descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = 1,
                .pImageInfo      = &gBuffer,
            },
            {
                .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet          = gdp->descriptorSet[i],
                .dstBinding      = 6,
                .dstArrayElement = 0,
                .descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = 1,
                .pImageInfo      = &nBuffer,
            },
        };
        vkUpdateDescriptorSets(device, 2, writes, 0, VK_NULL_HANDLE);
    }
}

void createDescriptorSetLayout(LogicalDevice* ld, GraphicPipelineToSurface* gdp, uint32_t maxFrames)
{

    VkDescriptorSetLayoutBinding uboInstanceTransform = {
        .binding            = 0,
        .descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount    = 1, // TODO! parametrizar, num uniform buffers
        .pImmutableSamplers = VK_NULL_HANDLE,
        .stageFlags         = VK_SHADER_STAGE_VERTEX_BIT,
    };

    VkDescriptorSetLayoutBinding samplerLayoutBinding = {
        .binding            = 1,
        .descriptorCount    = 1,
        .descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .pImmutableSamplers = VK_NULL_HANDLE,
        .stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT,
    };

    VkDescriptorSetLayoutBinding previousSSBOLayoutBinding = {
        .binding            = 2,
        .descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .descriptorCount    = 1,
        .pImmutableSamplers = VK_NULL_HANDLE,
        .stageFlags         = VK_SHADER_STAGE_COMPUTE_BIT,
    };

    VkDescriptorSetLayoutBinding currentSSBOLayoutBinding = {
        .binding            = 3,
        .descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .descriptorCount    = 1,
        .pImmutableSamplers = VK_NULL_HANDLE,
        .stageFlags         = VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT,
    };

    VkDescriptorSetLayoutBinding uboDeltaTime = {
        .binding            = 4,
        .descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount    = 1,
        .pImmutableSamplers = VK_NULL_HANDLE,
        .stageFlags         = VK_SHADER_STAGE_COMPUTE_BIT,
    };

    VkDescriptorSetLayoutBinding gBufferBinding = {
        .binding            = 5,
        .descriptorCount    = 1,
        .descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .pImmutableSamplers = VK_NULL_HANDLE,
        .stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT,
    };
    VkDescriptorSetLayoutBinding nBufferBinding = {
        .binding            = 6,
        .descriptorCount    = 1,
        .descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .pImmutableSamplers = VK_NULL_HANDLE,
        .stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT,
    };

    VkDescriptorSetLayoutBinding uboLightInstanceTransform = {
        .binding            = 7,
        .descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount    = 1, // TODO! parametrizar, num uniform buffers
        .pImmutableSamplers = VK_NULL_HANDLE,
        .stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT};

    VkDescriptorSetLayoutBinding ssboLightPixelsReadBack = {.binding            = 8,
                                                            .descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                                            .descriptorCount    = 1,
                                                            .pImmutableSamplers = VK_NULL_HANDLE,
                                                            .stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT};

    VkDescriptorSetLayoutBinding uboFrameGlobals = {
        .binding            = 9,
        .descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount    = 1,
        .pImmutableSamplers = VK_NULL_HANDLE,
        .stageFlags         = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
    };

    VkDescriptorSetLayoutBinding bindings[] = {
        uboInstanceTransform, samplerLayoutBinding, previousSSBOLayoutBinding, currentSSBOLayoutBinding, uboDeltaTime,
        gBufferBinding,       nBufferBinding,       uboLightInstanceTransform, ssboLightPixelsReadBack, uboFrameGlobals};

    VkDescriptorSetLayoutCreateInfo layoutInfo = {
        .sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = 10,
        .pBindings    = bindings,
        .pNext        = VK_NULL_HANDLE,
        .flags        = 0,
    };

    if (vkCreateDescriptorSetLayout(ld->device, &layoutInfo, VK_NULL_HANDLE, &gdp->dsl) != VK_SUCCESS)
    {

        exit(1);
    }
}
