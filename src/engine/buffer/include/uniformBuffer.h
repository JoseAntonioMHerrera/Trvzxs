#pragma once
#include <time.h>
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
    glm_rotate(ubo.model, time * glm_rad(45.0f), (vec3){0.0f, 0.0f, 1.0f});

    // View matrix: camera at (2,2,2) looking at (0,0,0) with up vector (0,0,1)
    vec3 eye = {4.0f, 4.0f, 4.0f};
    vec3 center = {0.0f, 0.0f, 0.0f};
    vec3 up = {0.0f, 0.0f, 1.0f};

    glm_lookat(eye, center, up, ubo.view);

    // Projection matrix: 45 degree FOV, aspect ratio based on swap chain extent

    glm_perspective(glm_rad(45.0f), dsm->currentExtent2D.width / (float) dsm->currentExtent2D.height, 0.1f, 100.0f, ubo.proj);
    //printf("WHAT Is happening HERE \n");
    // Adjust for Vulkan's coordinate system
    ubo.proj[1][1] *= -1;

    return ubo;
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
