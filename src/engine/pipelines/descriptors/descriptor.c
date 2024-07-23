
#include "descriptor.h"

void createDescriptorPool(uint32_t maxFrames, VkDevice device, GeometryDisplayPipelineManager* gdp){
    VkDescriptorPoolSize poolSizes[] = {
        {
            .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = maxFrames,
        },
        {
            .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = maxFrames,
        }
    };
    VkDescriptorPoolCreateInfo poolInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .poolSizeCount = 2,
        .pPoolSizes = poolSizes,
        .maxSets = maxFrames,
        .flags = 0,
        .pNext = VK_NULL_HANDLE,
    };

    poolInfo.maxSets = maxFrames;
    //TODO moved descriptorPool to stack
    // gdp->descriptorPool = malloc(sizeof(*gdp->descriptorPool));
    if (vkCreateDescriptorPool(device, &poolInfo, VK_NULL_HANDLE, &(gdp->descriptorPool)) != VK_SUCCESS) {
        printf("failed to create descriptor pool!\n");
    }

}

void createDescriptorSet(uint32_t maxFrames, VkDevice device, GeometryDisplayPipelineManager* gdp, Texture2D* texture){
    assert(gdp->dsl);

    // We allocate one descriptorset for
    gdp->descriptorSet = malloc(sizeof(*gdp->descriptorSet) * maxFrames);
    printf("%u\n", maxFrames);

    VkDescriptorSetAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = gdp->descriptorPool,
        .descriptorSetCount = maxFrames,
        .pSetLayouts = (VkDescriptorSetLayout[]){gdp->dsl, gdp->dsl, gdp->dsl},
    };
    printf("CREATE DESCRIPTOR SET FLAG 1\n");
    if (vkAllocateDescriptorSets(device, &allocInfo, gdp->descriptorSet) != VK_SUCCESS) {
        printf("failed to allocate descriptors sets!");
    }
    printf("CREATE DESCRIPTOR SET FLAG 1\n");
    for (size_t i = 0; i < maxFrames; i++) {
        VkDescriptorBufferInfo bufferInfo = {
            .buffer = gdp->deviceBuffer->ubo->uniformBuffers[i],
            .offset = 0,
            .range = sizeof(MODEL_VIEW_PROJECTION),
        };
        VkDescriptorImageInfo imgInfo = {
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .imageView = texture->imgView,
            .sampler = texture->sampler,
        };

        VkWriteDescriptorSet descriptorWrite[] = {
            {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = gdp->descriptorSet[i],
                .dstBinding = 0,
                .dstArrayElement = 0,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .descriptorCount = 1,
                .pBufferInfo = &bufferInfo,
                .pImageInfo = VK_NULL_HANDLE, // Optional
                .pTexelBufferView = VK_NULL_HANDLE, // Optional
            },
            {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = gdp->descriptorSet[i],
                .dstBinding = 1,
                .dstArrayElement = 0,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = 1,
                .pImageInfo = &imgInfo,
                .pTexelBufferView = VK_NULL_HANDLE, // Optional
            },
        };

        vkUpdateDescriptorSets(device, 2, descriptorWrite, 0, VK_NULL_HANDLE);
        printf("ACTUALIZAMOS DESCRIPTORSETS\n");
    }
}

void createDescriptorSetLayout(LogicalDevice *ld, GeometryDisplayPipelineManager* gdp, uint32_t maxFrames){

    VkDescriptorSetLayoutBinding uboLayoutBinding = {
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1,
        .pImmutableSamplers = VK_NULL_HANDLE,
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
    };
    VkDescriptorSetLayoutBinding samplerLayoutBinding = {
        .binding = 1,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .pImmutableSamplers = VK_NULL_HANDLE,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
    };


    VkDescriptorSetLayoutBinding bindings[] = {uboLayoutBinding, samplerLayoutBinding};

    printf("DESCRIPTORSETLAYOUTF LAG 1\n");
    VkDescriptorSetLayoutCreateInfo layoutInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = 2,
        .pBindings = bindings,
        .pNext = VK_NULL_HANDLE,
        .flags = 0,
    };
    printf("DESCRIPTORSETLAYOUTF LAG 2\n");
    printf("%u\n", maxFrames);

    if (vkCreateDescriptorSetLayout(ld->device, &layoutInfo, VK_NULL_HANDLE, &gdp->dsl) != VK_SUCCESS) {
        printf("failed to create descriptor set layout!\n");
        exit(1);
    }

    printf("Created descriptor set layout: %p\n", (void*)gdp->dsl);

}
