
#include "descriptor.h"
#include "vertex.h"
#include <assert.h>
#include "particle.h"
void createDescriptorPool(uint32_t maxFrames, VkDevice device, GraphicPipelineToSurface* gdp)
{
    VkDescriptorPoolSize       poolSizes[] = {{
                                                  .type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                                  .descriptorCount = maxFrames,
                                        },
                                              {
                                                  .type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                                  .descriptorCount = maxFrames,
                                        },
                                              {
                                                  .type            = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                                  .descriptorCount = maxFrames,
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
        printf("FAILED AT CREATION vkAllocateDescriptorSets\n");
    }

    for (size_t i = 0; i < maxFrames; i++)
    {
        printf("HOLA HOLA %lu\n", (i - 1 + maxFrames) % maxFrames);
        VkDescriptorBufferInfo transformUBOInfo[2] = {{
                                                          .buffer = gdp->deviceBuffer->pUbo->uniformBuffers[i],
                                                          .offset = 0,
                                                          .range  = sizeof(MODEL_VIEW_PROJECTION),
                                                      },
                                                      {
                                                          .buffer = lightGraphic->deviceBuffer->pUbo->uniformBuffers[i],
                                                          .offset = 0,
                                                          .range  = sizeof(MODEL_VIEW_PROJECTION),
                                                      }};

        VkDescriptorBufferInfo deltaUBOInfo = {
            .buffer = gdp->deviceBuffer->pUbo->uniformBuffers[i],
            .offset = 0,
            .range  = sizeof(float),
        };

        VkDescriptorBufferInfo prevSSBOBufferInfo = {
            .buffer = particleGraphic->computeP->deviceBuffer->ssbos->buffers[(i - 1 + maxFrames) % maxFrames],
            .offset = 0,
            .range  = sizeof(GEOM_PARTICLE) * 1024,
        };

        VkDescriptorBufferInfo currentSSBOBufferInfo = {
            .buffer = particleGraphic->computeP->deviceBuffer->ssbos->buffers[i],
            .offset = 0,
            .range  = sizeof(GEOM_PARTICLE) * 1024,
        };
        VkDescriptorImageInfo imgInfo = {
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .imageView   = texture->common->imageView,
            .sampler     = texture->sampler,
        };

        VkDescriptorImageInfo gBuffer = {
            .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
            .imageView   = gdp->swapchain->gBuffer->imageView,
        };
        VkDescriptorImageInfo nBuffer = {
            .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
            .imageView   = gdp->swapchain->nBuffer->imageView,
        };
        printf("ADIOS ADIOS\n");
        VkWriteDescriptorSet descriptorWrite[] = {
            {
                .sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet           = gdp->descriptorSet[i],
                .dstBinding       = 0,
                .dstArrayElement  = 0,
                .descriptorType   = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .descriptorCount  = 2,
                .pBufferInfo      = transformUBOInfo,
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
                .descriptorType   = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
                .descriptorCount  = 1,
                .pBufferInfo      = VK_NULL_HANDLE,
                .pImageInfo       = &gBuffer,       // Optional
                .pTexelBufferView = VK_NULL_HANDLE, // Optional
            },
            {
                .sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet           = gdp->descriptorSet[i],
                .dstBinding       = 6,
                .dstArrayElement  = 0,
                .descriptorType   = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
                .descriptorCount  = 1,
                .pBufferInfo      = VK_NULL_HANDLE,
                .pImageInfo       = &nBuffer,       // Optional
                .pTexelBufferView = VK_NULL_HANDLE, // Optional
            },
        };
        printf("UPDATE\n");
        vkUpdateDescriptorSets(device, 7, descriptorWrite, 0, VK_NULL_HANDLE);
    }
}

void createDescriptorSetLayout(LogicalDevice* ld, GraphicPipelineToSurface* gdp, uint32_t maxFrames)
{

    VkDescriptorSetLayoutBinding uboInstanceTransform = {
        .binding            = 0,
        .descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount    = 2,
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
        .descriptorType     = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
        .pImmutableSamplers = VK_NULL_HANDLE,
        .stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT,
    };
    VkDescriptorSetLayoutBinding nBufferBinding = {
        .binding            = 6,
        .descriptorCount    = 1,
        .descriptorType     = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
        .pImmutableSamplers = VK_NULL_HANDLE,
        .stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT,
    };

    VkDescriptorSetLayoutBinding bindings[] = {
        uboInstanceTransform, samplerLayoutBinding, previousSSBOLayoutBinding, currentSSBOLayoutBinding, uboDeltaTime,
        gBufferBinding,       nBufferBinding};

    VkDescriptorSetLayoutCreateInfo layoutInfo = {
        .sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = 7,
        .pBindings    = bindings,
        .pNext        = VK_NULL_HANDLE,
        .flags        = 0,
    };

    if (vkCreateDescriptorSetLayout(ld->device, &layoutInfo, VK_NULL_HANDLE, &gdp->dsl) != VK_SUCCESS)
    {

        exit(1);
    }
}
