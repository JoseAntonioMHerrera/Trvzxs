#pragma once
#include <vulkan/vulkan.h>
#include <stddef.h>
#include <stdio.h>
#include "../../../include/geometry/transformations.h"
#include "../../../include/geometry/vertex.h"
#include "../../../include/langDefinitions/compilerMacros.h"
#include "../../../include/langDefinitions/numberTypes.h"
#include "../../../include/langDefinitions/miscTypes.h"
#include "../../device/include/deviceManagment.h"
#include "../../commands/include/commandBuffer.h"
#include "buffer.h"
#include <string.h> //TODO revisar memcpy de aqui

typedef struct UniformBuffersManager {
    VkBuffer *uniformBuffers;
    uint32_t uniformBuffersLen;
    VkDeviceMemory *uniformBuffersMemory;
    void* uniformBuffersMapped;
    void* uniformBufferType;
    VkDescriptorSetLayout *dsl;
} UniformBuffersManager
;
TRVZXS_inline void createDescriptorSetLayout(LogicalDevice *ld, UniformBuffersManager* ubos){
    VkDescriptorSetLayoutBinding uboLayoutBinding ={
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1,
        .pImmutableSamplers = VK_NULL_HANDLE,
    };
    VkDescriptorSetLayoutCreateInfo layoutInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = 1,
        .pBindings = &uboLayoutBinding,
     };


    if (vkCreateDescriptorSetLayout(ld->device, &layoutInfo, VK_NULL_HANDLE, ubos->dsl) != VK_SUCCESS) {
        printf("failed to create descriptor set layout!\n");
        exit(1);
    }

}

TRVZXS_inline int createUniformBuffers(PhysicalDevice *pd, LogicalDevice *ld, const uint32_t len, UniformBuffersManager *ubos, char* error) {

    assert(ubos);

    if(!ubos->uniformBufferType){

        sprintf("Error! TRVZXS_UBOS *(%x) struct should have defined uniformBufferType!", error, ubos);
        exit(1);
    }

    VkDeviceSize bufferSize = sizeof(MODEL_VIEW_PROJECTION);

    uint32_t uniformBuffersLen = len;
    if(uniformBuffersLen < 1){
        uniformBuffersLen = 1;
    }

    ubos->uniformBuffers = (VkBuffer*) malloc(sizeof(VkBuffer)*uniformBuffersLen);
    ubos->uniformBuffersMemory = (VkDeviceMemory*) malloc(sizeof(VkDeviceMemory)*uniformBuffersLen);
    ubos->uniformBuffersLen = uniformBuffersLen;


    for (TRVZXS_SIZE i = 0; i < uniformBuffersLen; i++) {
        createVkBuffer(pd, ld->device, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &ubos->uniformBuffers[i], &ubos->uniformBuffersMemory[i]);
    }
    return 1;
};
