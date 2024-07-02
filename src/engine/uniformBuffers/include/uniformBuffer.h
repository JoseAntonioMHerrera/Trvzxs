#ifndef _UBO_H_
#define _UBO_H_

#include "vulkan.h"
#include "transformations.h"
#include "compilerMacros.h"
#include "numberTypes.h"
#include <stddef.h>
#include <stdio.h>


typedef struct TRVZXS_UBOS {
    VkBuffer *uniformBuffers;
    uint32_t uniformBuffersLen;
    VkDeviceMemory *uniformBuffersMemory;
    void* uniformBuffersMapped;
    void* uniformBufferType;
};

TRVZXS_inline int createUniformBuffers(const uint32_t len, TRVZXS_UBOS *ubos, char* error) {

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
        createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffers[i], uniformBuffersMemory[i]);

        vkMapMemory(device, uniformBuffersMemory[i], 0, bufferSize, 0, &uniformBuffersMapped[i]);
    }
    return 1;
};

#endif
