#include "uniformBuffer.h"
#include "camera.h"

MODEL_VIEW_PROJECTION* updateUniformBuffers(GraphicSwapchain* dsm, float deltaTime, Camera3D camera, SCENE* scene) {

    static struct timespec startTime;
    static int initialized = 0;
    struct timespec currentTime;
    clock_gettime(CLOCK_MONOTONIC, &currentTime);

    // Calculate the difference in seconds

    float time = (currentTime.tv_sec - startTime.tv_sec) + (currentTime.tv_nsec - startTime.tv_nsec) / 1e9f;
    uint32_t transformationsLen = scene->modelsManager.modelInstanceLen + scene->lightManager.lightInstanceLen;
    MODEL_VIEW_PROJECTION* pTransformations = malloc(sizeof(MODEL_VIEW_PROJECTION) * transformationsLen);

    for(uint32_t matIdx = 0; matIdx < scene->modelsManager.modelInstanceLen; matIdx++){
    // Model matrix: rotate around Z-axis
        glm_mat4_copy(scene->modelsManager.pModelInstance[matIdx].m, pTransformations[matIdx].model);
        //glm_rotate(ubo.model, time * glm_rad(45.0f), (vec3){0.0f, 1.0f, 0.0f});

        // View matrix: camera at (2,2,2) looking at (0,0,0) with up vector (0,0,1)
        vec3 eye = {1.5f, 1.5f, 1.5f};
        vec3 center = {0.0f, 0.0f, 0.0f};
        vec3 up = {0.0f, 0.0f, 1.0f};
        vec4 view;
        glm_lookat(eye, center, up, pTransformations[matIdx].view);
        // Projection matrix: 45 degree FOV, aspect ratio based on swap chain extent
        vec4 proj;
        glm_perspective(glm_rad(45.0f), dsm->currentExtent2D.width / (float) dsm->currentExtent2D.height, 0.1f, 500.0f, pTransformations[matIdx].proj);
    //printf("WHAT Is happening HERE \n");
        // Adjust for Vulkan's coordinate system
        pTransformations[matIdx].proj[1][1] *= -1;
    }


    return pTransformations;
}

void freeMappedUniformBufferLocalMemory(mat4** instancedTranformations){
    free(*instancedTranformations);
    *instancedTranformations = 0;
}
//
int createModelInstanceTransformationsUniformBuffersMapped(PhysicalDevice *pd, LogicalDevice *ld, const uint32_t ubosLen, UniformBufferManager *ubos, uint32_t modelInstanceLen) {

    assert(ubos);


    VkDeviceSize bufferSize = sizeof(MODEL_VIEW_PROJECTION) * modelInstanceLen;

    ubos->uniformBuffersMapped = malloc(sizeof(*ubos->uniformBuffersMapped) * ubosLen);

    uint32_t uniformBuffersLen = ubosLen;
    if(uniformBuffersLen < 1){
        uniformBuffersLen = 1;
    }

    ubos->uniformBuffers = (VkBuffer*) malloc(sizeof(VkBuffer)*uniformBuffersLen);
    ubos->uniformBuffersMemory = (VkDeviceMemory*) malloc(sizeof(VkDeviceMemory)*uniformBuffersLen);
    ubos->uniformBuffersLen = uniformBuffersLen;


    for (TRVZXS_SIZE i = 0; i < uniformBuffersLen; i++) {
        createVkBuffer(pd, ld->device, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_SHARING_MODE_EXCLUSIVE,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &ubos->uniformBuffers[i],
        &ubos->uniformBuffersMemory[i]);
        vkMapMemory(ld->device, ubos->uniformBuffersMemory[i], 0, bufferSize, 0, &ubos->uniformBuffersMapped[i]);

    }
    printf("EN CREATEUNIFORMBUFFERSMAPPED AL FINAL\n");
    return 1;
};

int createDeltaTimeUniformBuffers(PhysicalDevice *pd, LogicalDevice *ld, const uint32_t ubosLen, UniformBufferManager *ubos) {

    assert(ubos);

    VkDeviceSize bufferSize = sizeof(UboDeltaTime);

    uint32_t uniformBuffersLen = ubosLen;
    if(uniformBuffersLen < 1){
        uniformBuffersLen = 1;
    }

    ubos->uniformBuffers = (VkBuffer*) malloc(sizeof(VkBuffer)*uniformBuffersLen);
    ubos->uniformBuffersMemory = (VkDeviceMemory*) malloc(sizeof(VkDeviceMemory)*uniformBuffersLen);
    ubos->uniformBuffersLen = uniformBuffersLen;

    for (TRVZXS_SIZE i = 0; i < uniformBuffersLen; i++) {
        createVkBuffer(pd, ld->device, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_SHARING_MODE_EXCLUSIVE,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &ubos->uniformBuffers[i],
        &ubos->uniformBuffersMemory[i]);
    }

    printf("EN CREATEUNIFORMBUFFERS AL FINAL\n");
    return 1;
};
