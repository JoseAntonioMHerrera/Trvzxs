#pragma once
#include <vulkan/vulkan.h>
#include <stddef.h>
#include <stdio.h>
#include "transformations.h"
#include "vertex.h"
#include "compilerMacros.h"
#include "numberTypes.h"
#include "miscTypes.h"
#include "deviceManagment.h"
#include "commandBuffer.h"
#include "buffer.h"
#include <string.h> //TODO revisar memcpy de aqui
#include "bufferTypes.h"
#include "cam.h"
#include "mat4.h"
#include "affine.h"
//#include "displayTimer.h"
#include <time.h>

TRVZXS_inline void createDescriptorPool(uint32_t maxFrames, VkDevice device, UniformBufferManager* ubo){
    VkDescriptorPoolSize poolSize = {
        .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = maxFrames,
    };

    VkDescriptorPoolCreateInfo poolInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .poolSizeCount = 1,
        .pPoolSizes = &poolSize,
        .flags = 0,
        .pNext = VK_NULL_HANDLE,
    };

    poolInfo.maxSets = maxFrames;

    ubo->descriptorPool = malloc(sizeof(*ubo->descriptorPool));
    if (vkCreateDescriptorPool(device, &poolInfo, VK_NULL_HANDLE, ubo->descriptorPool) != VK_SUCCESS) {
        printf("failed to create descriptor pool!\n");
    }

}

TRVZXS_inline void createDescriptorSet(uint32_t maxFrames, VkDevice device, UniformBufferManager *ubo){
    assert(ubo->dsl);

    ubo->descriptorSet = malloc(sizeof(ubo->descriptorSet) * maxFrames);

    VkDescriptorSetAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = *ubo->descriptorPool,
        .descriptorSetCount = maxFrames,
        .pSetLayouts = ubo->dsl,
    };

    if (vkAllocateDescriptorSets(device, &allocInfo, ubo->descriptorSet) != VK_SUCCESS) {
        printf("failed to allocate descriptors sets!");
    }

    for (size_t i = 0; i < maxFrames; i++) {
        VkDescriptorBufferInfo bufferInfo = {
            bufferInfo.buffer = ubo->uniformBuffers[i],
            bufferInfo.offset = 0,
            bufferInfo.range = sizeof(MODEL_VIEW_PROJECTION),
        };
        VkWriteDescriptorSet descriptorWrite = {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = ubo->descriptorSet[i],
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .pBufferInfo = &bufferInfo,
            .pImageInfo = VK_NULL_HANDLE, // Optional
            .pTexelBufferView = VK_NULL_HANDLE, // Optional
        };

        vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, VK_NULL_HANDLE);
        printf("ACTUALIZAMOS DESCRIPTORSETS\n");
    }

}

TRVZXS_inline MODEL_VIEW_PROJECTION updateUniformBuffers(DisplaySwapchainManager* dsm) {

    static struct timespec startTime;
    static int initialized = 0;
    struct timespec currentTime;
    clock_gettime(CLOCK_MONOTONIC, &currentTime);

    // Calculate the difference in seconds

    float time = (currentTime.tv_sec - startTime.tv_sec) + (currentTime.tv_nsec - startTime.tv_nsec) / 1e9f;
    MODEL_VIEW_PROJECTION ubo;

    // Model matrix: rotate around Z-axis
    glm_mat4_identity(ubo.model);
    glm_rotate(ubo.model, time * glm_rad(90.0f), (vec3){0.0f, 0.0f, 1.0f});

    // View matrix: camera at (2,2,2) looking at (0,0,0) with up vector (0,0,1)
    vec3 eye = {3.0f, 3.0f, 3.0f};
    vec3 center = {1.0f, 2.0f, 2.0f};
    vec3 up = {0.0f, 0.0f, 1.0f};
    glm_lookat(eye, center, up, ubo.view);

    // Projection matrix: 45 degree FOV, aspect ratio based on swap chain extent

    glm_perspective(glm_rad(45.0f), dsm->currentExtent2D.width / (float) dsm->currentExtent2D.height, 0.1f, 10.0f, ubo.proj);
    printf("WHAT Is happening HERE \n");
    // Adjust for Vulkan's coordinate system
    ubo.proj[1][1] *= -1;

    return ubo;
}
TRVZXS_inline void createDescriptorSetLayout(LogicalDevice *ld, UniformBufferManager* ubos, size_t maxFrames){

    ubos->dsl = malloc(sizeof(*(ubos->dsl))*maxFrames);

    assert(ubos->dsl);

    VkDescriptorSetLayoutBinding uboLayoutBinding ={
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1,
        .pImmutableSamplers = VK_NULL_HANDLE,
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
    };
    printf("DESCRIPTORSETLAYOUTF LAG 1\n");
    VkDescriptorSetLayoutCreateInfo layoutInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = 1,
        .pBindings = &uboLayoutBinding,
        .pNext = VK_NULL_HANDLE,
    };
    printf("DESCRIPTORSETLAYOUTF LAG 2\n");

    if (vkCreateDescriptorSetLayout(ld->device, &layoutInfo, VK_NULL_HANDLE, ubos->dsl) != VK_SUCCESS) {
        printf("failed to create descriptor set layout!\n");
        exit(1);
    }


    for(uint32_t i = 1; i < maxFrames; i++)
        ubos->dsl[i] = ubos->dsl[0];

}

TRVZXS_inline int createUniformBuffers(PhysicalDevice *pd, LogicalDevice *ld, const uint32_t ubosLen, UniformBufferManager *ubos) {
    printf("holahsodasdsa\n");
    assert(ubos);
    printf("holahsodasdsa\n");
    printf("holahsodasdsa\n");
    VkDeviceSize bufferSize = sizeof(MODEL_VIEW_PROJECTION);

    ubos->uniformBuffersMapped = malloc(sizeof(*ubos->uniformBuffersMapped) * ld->displayManager->geomPipeline->maxFrames);
    printf("holahsodasdsa\n");
    uint32_t uniformBuffersLen = ubosLen;
    if(uniformBuffersLen < 1){
        uniformBuffersLen = 1;
    }
    printf("holahsodasdsa\n");
    ubos->uniformBuffers = (VkBuffer*) malloc(sizeof(VkBuffer)*uniformBuffersLen);
    ubos->uniformBuffersMemory = (VkDeviceMemory*) malloc(sizeof(VkDeviceMemory)*uniformBuffersLen);
    ubos->uniformBuffersLen = uniformBuffersLen;
    printf("holahsodasdsa\n");

    for (TRVZXS_SIZE i = 0; i < uniformBuffersLen; i++) {
        createVkBuffer(pd, ld->device, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_SHARING_MODE_EXCLUSIVE,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &ubos->uniformBuffers[i],
        &ubos->uniformBuffersMemory[i]);
        vkMapMemory(ld->device, ubos->uniformBuffersMemory[i], 0, bufferSize, 0, &ubos->uniformBuffersMapped[i]);
        printf("asdhaiodnhaisdnaisndaio\n");
    }
    return 1;
};
