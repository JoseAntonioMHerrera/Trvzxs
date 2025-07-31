#include "uniformBuffer.h"
#include "camera.h"

MODEL_VIEW_PROJECTION* updateModelUniformBuffers(GraphicSwapchain* dsm, float deltaTime, Camera3D camera, SCENE* scene)
{
    printf("=== UPDATING UNIFORM BUFFERS ===\n");
    printf("Delta time: %.4f\n", deltaTime);

    static struct timespec startTime;
    static int             initialized = 0;
    struct timespec        currentTime;
    clock_gettime(CLOCK_MONOTONIC, &currentTime);

    // Calculate the difference in seconds
    float time = (currentTime.tv_sec - startTime.tv_sec) + (currentTime.tv_nsec - startTime.tv_nsec) / 1e9f;
    printf("Elapsed time: %.4f seconds\n", time);

    uint32_t transformationsCount = scene->modelsManager.modelInstanceLen;
    printf("Total transformations needed: %u (models: %u + lights: %u)\n", transformationsCount,
           scene->modelsManager.modelInstanceLen, scene->lightManager.lightInstanceCount);

    MODEL_VIEW_PROJECTION* pTransformations = malloc(sizeof(MODEL_VIEW_PROJECTION) * transformationsCount);
    printf("Allocated transformations array at %p (size: %zu bytes)\n", pTransformations,
           sizeof(MODEL_VIEW_PROJECTION) * transformationsCount);
    uint32_t transformationsIdx = 0;
    printf("Processing %u model instances:\n", scene->modelsManager.modelInstanceLen);
    for (uint32_t modelIdx = 0; modelIdx < scene->modelsManager.modelLen; modelIdx++)
    {
        Model3D model = scene->modelsManager.pModel[modelIdx];
        for (uint32_t instanceIdx = 0; instanceIdx < model.instanceCount; instanceIdx++)
        {
            // Model matrix: rotate around Z-axis
            glm_mat4_copy(model.pModelInstance[instanceIdx].m, pTransformations[transformationsIdx].model);
            printf("    Model matrix copied from instance %u\n", transformationsIdx);
            // glm_rotate(ubo.model, time * glm_rad(45.0f), (vec3){0.0f, 1.0f, 0.0f});

            // View matrix: camera at (2,2,2) looking at (0,0,0) with up vector (0,0,1)
            vec3 eye    = {1.5f, 1.5f, 1.5f};
            vec3 center = {0.0f, 0.0f, 0.0f};
            vec3 up     = {0.0f, 0.0f, 1.0f};
            printf("    Setting up view matrix: eye(%.1f,%.1f,%.1f) center(%.1f,%.1f,%.1f) up(%.1f,%.1f,%.1f)\n",
                   eye[0], eye[1], eye[2], center[0], center[1], center[2], up[0], up[1], up[2]);

            vec4 view;
            glm_lookat(eye, center, up, pTransformations[transformationsIdx].view);
            printf("    View matrix calculated\n");

            // Projection matrix: 45 degree FOV, aspect ratio based on swap chain extent
            float aspectRatio = dsm->currentExtent2D.width / (float) dsm->currentExtent2D.height;
            printf("    Projection setup: FOV=45°, aspect=%.3f (%ux%u), near=0.1, far=500.0\n", aspectRatio,
                   dsm->currentExtent2D.width, dsm->currentExtent2D.height);

            vec4 proj;
            glm_perspective(glm_rad(45.0f), aspectRatio, 0.1f, 500.0f, pTransformations[transformationsIdx].proj);
            printf("    Projection matrix calculated\n");

            // Adjust for Vulkan's coordinate system
            pTransformations[transformationsIdx].proj[1][1] *= -1;
            printf("    Vulkan Y-axis flip applied (proj[1][1] *= -1)\n");
            transformationsIdx++;
            printf("  Model instance %u transformation complete\n", transformationsIdx);
        }
    }

    printf("All transformations processed successfully\n");
    printf("Returning transformations array: %p\n", pTransformations);
    printf("=== UNIFORM BUFFER UPDATE COMPLETE ===\n");
    return pTransformations;
}

MODEL_VIEW_PROJECTION* updateLightUniformBuffers(GraphicSwapchain* dsm, float deltaTime, Camera3D camera, SCENE* scene)
{
    printf("=== UPDATING UNIFORM BUFFERS ===\n");
    printf("Delta time: %.4f\n", deltaTime);

    static struct timespec startTime;
    static int             initialized = 0;
    struct timespec        currentTime;
    clock_gettime(CLOCK_MONOTONIC, &currentTime);

    // Calculate the difference in seconds
    float time = (currentTime.tv_sec - startTime.tv_sec) + (currentTime.tv_nsec - startTime.tv_nsec) / 1e9f;
    printf("Elapsed time: %.4f seconds\n", time);

    uint32_t transformationsLen = scene->lightManager.lightInstanceCount;
    printf("Total transformations needed: %u (models: %u + lights: %u)\n", transformationsLen,
           scene->modelsManager.modelInstanceLen, scene->lightManager.lightInstanceCount);

    MODEL_VIEW_PROJECTION* pTransformations = malloc(sizeof(MODEL_VIEW_PROJECTION) * transformationsLen);
    printf("Allocated transformations array at %p (size: %zu bytes)\n", pTransformations,
           sizeof(MODEL_VIEW_PROJECTION) * transformationsLen);

    printf("Processing %u model instances:\n", scene->modelsManager.modelInstanceLen);
    uint32_t transformationsCount = 0;
    for (uint32_t lightIdx = 0; lightIdx < scene->modelsManager.modelLen; lightIdx++)
    {
        LightVolume light = scene->lightManager.pLight[lightIdx];
        for (uint32_t instanceIdx = 0; instanceIdx < light.lightVolumeInstanceCount; instanceIdx++)
        {
            // Model matrix: rotate around Z-axis
            glm_mat4_copy(light.pLightVolumeInstance[instanceIdx].m, pTransformations[transformationsCount].model);
            printf("    Model matrix copied from instance %u\n", transformationsCount);
            // glm_rotate(ubo.model, time * glm_rad(45.0f), (vec3){0.0f, 1.0f, 0.0f});

            // View matrix: camera at (2,2,2) looking at (0,0,0) with up vector (0,0,1)
            vec3 eye    = {1.5f, 1.5f, 1.5f};
            vec3 center = {0.0f, 0.0f, 0.0f};
            vec3 up     = {0.0f, 0.0f, 1.0f};
            printf("    Setting up view matrix: eye(%.1f,%.1f,%.1f) center(%.1f,%.1f,%.1f) up(%.1f,%.1f,%.1f)\n",
                   eye[0], eye[1], eye[2], center[0], center[1], center[2], up[0], up[1], up[2]);

            vec4 view;
            glm_lookat(eye, center, up, pTransformations[transformationsCount].view);
            printf("    View matrix calculated\n");

            // Projection matrix: 45 degree FOV, aspect ratio based on swap chain extent
            float aspectRatio = dsm->currentExtent2D.width / (float) dsm->currentExtent2D.height;
            printf("    Projection setup: FOV=45°, aspect=%.3f (%ux%u), near=0.1, far=500.0\n", aspectRatio,
                   dsm->currentExtent2D.width, dsm->currentExtent2D.height);

            vec4 proj;
            glm_perspective(glm_rad(45.0f), aspectRatio, 0.1f, 500.0f, pTransformations[transformationsCount].proj);
            printf("    Projection matrix calculated\n");

            // Adjust for Vulkan's coordinate system
            pTransformations[transformationsCount].proj[1][1] *= -1;
            printf("    Vulkan Y-axis flip applied (proj[1][1] *= -1)\n");

            printf("  Model instance %u transformation complete\n", transformationsCount);
        }
    }

    printf("All transformations processed successfully\n");
    printf("Returning transformations array: %p\n", pTransformations);
    printf("=== UNIFORM BUFFER UPDATE COMPLETE ===\n");
    return pTransformations;
}

void freeMappedUniformBufferLocalMemory(mat4** instancedTranformations)
{
    free(*instancedTranformations);
    *instancedTranformations = 0;
}
//
int createModelInstanceTransformationsUniformBuffersMapped(PhysicalDevice* pd, LogicalDevice* ld,
                                                           const uint32_t ubosLen, UniformBufferManager* ubos,
                                                           uint32_t modelInstanceLen)
{
    printf("=== CREATING MODEL INSTANCE TRANSFORMATION UNIFORM BUFFERS ===\n");
    printf("Input parameters: ubosLen=%u, modelInstanceLen=%u\n", ubosLen, modelInstanceLen);

    assert(ubos);

    VkDeviceSize bufferSize = sizeof(MODEL_VIEW_PROJECTION) * modelInstanceLen;
    printf("Buffer size calculation: sizeof(MODEL_VIEW_PROJECTION)=%zu * modelInstanceLen=%u = %llu bytes\n",
           sizeof(MODEL_VIEW_PROJECTION), modelInstanceLen, (unsigned long long) bufferSize);

    ubos->uniformBuffersMapped = malloc(sizeof(*ubos->uniformBuffersMapped) * ubosLen);
    printf("Allocated uniformBuffersMapped array at %p for %u pointers\n", ubos->uniformBuffersMapped, ubosLen);

    uint32_t uniformBuffersLen = ubosLen;
    printf("Original uniformBuffersLen: %u\n", uniformBuffersLen);

    if (uniformBuffersLen < 1)
    {
        printf("Adjusting uniformBuffersLen from %u to 1\n", uniformBuffersLen);
        uniformBuffersLen = 1;
    }

    ubos->uniformBuffers = (VkBuffer*) malloc(sizeof(VkBuffer) * uniformBuffersLen);
    printf("Allocated uniformBuffers array at %p for %u buffers\n", ubos->uniformBuffers, uniformBuffersLen);

    ubos->uniformBuffersMemory = (VkDeviceMemory*) malloc(sizeof(VkDeviceMemory) * uniformBuffersLen);
    printf("Allocated uniformBuffersMemory array at %p for %u memories\n", ubos->uniformBuffersMemory,
           uniformBuffersLen);

    ubos->uniformBuffersLen = uniformBuffersLen;
    printf("Set uniformBuffersLen to: %u\n", ubos->uniformBuffersLen);

    for (TRVZXS_SIZE i = 0; i < uniformBuffersLen; i++)
    {
        printf("Creating buffer %zu/%u with size %llu bytes\n", i, uniformBuffersLen - 1,
               (unsigned long long) bufferSize);

        createVkBuffer(pd, ld->device, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE,
                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                       &ubos->uniformBuffers[i], &ubos->uniformBuffersMemory[i]);

        printf("Buffer %zu created: handle=%p, memory=%p\n", i, (void*) (uintptr_t) ubos->uniformBuffers[i],
               (void*) (uintptr_t) ubos->uniformBuffersMemory[i]);

        vkMapMemory(ld->device, ubos->uniformBuffersMemory[i], 0, bufferSize, 0, &ubos->uniformBuffersMapped[i]);

        printf("Buffer %zu mapped at address: %p\n", i, ubos->uniformBuffersMapped[i]);
    }

    printf("flag createModelInstanceTransformationsUniformBuffersMapped\n");
    printf("=== UNIFORM BUFFER CREATION COMPLETE ===\n");
    return 1;
}

int createLightInstanceTransformationsUniformBuffersMapped(PhysicalDevice* pd, LogicalDevice* ld,
                                                           const uint32_t ubosLen, UniformBufferManager* ubos,
                                                           uint32_t lightInstanceCount)
{
    printf("=== CREATING LIGHT INSTANCE TRANSFORMATION UNIFORM BUFFERS ===\n");
    printf("Input parameters: ubosLen=%u, modelInstanceLen=%u\n", ubosLen, lightInstanceCount);

    assert(ubos);

    VkDeviceSize bufferSize = sizeof(MODEL_VIEW_PROJECTION) * lightInstanceCount;
    printf("Buffer size calculation: sizeof(MODEL_VIEW_PROJECTION)=%zu * modelInstanceLen=%u = %llu bytes\n",
           sizeof(MODEL_VIEW_PROJECTION), lightInstanceCount, (unsigned long long) bufferSize);

    ubos->uniformBuffersMapped = malloc(sizeof(*ubos->uniformBuffersMapped) * ubosLen);
    printf("Allocated uniformBuffersMapped array at %p for %u pointers\n", ubos->uniformBuffersMapped, ubosLen);

    uint32_t uniformBuffersLen = ubosLen;
    printf("Original uniformBuffersLen: %u\n", uniformBuffersLen);

    if (uniformBuffersLen < 1)
    {
        printf("Adjusting uniformBuffersLen from %u to 1\n", uniformBuffersLen);
        uniformBuffersLen = 1;
    }

    ubos->uniformBuffers = (VkBuffer*) malloc(sizeof(VkBuffer) * uniformBuffersLen);
    printf("Allocated uniformBuffers array at %p for %u buffers\n", ubos->uniformBuffers, uniformBuffersLen);

    ubos->uniformBuffersMemory = (VkDeviceMemory*) malloc(sizeof(VkDeviceMemory) * uniformBuffersLen);
    printf("Allocated uniformBuffersMemory array at %p for %u memories\n", ubos->uniformBuffersMemory,
           uniformBuffersLen);

    ubos->uniformBuffersLen = uniformBuffersLen;
    printf("Set uniformBuffersLen to: %u\n", ubos->uniformBuffersLen);

    for (TRVZXS_SIZE i = 0; i < uniformBuffersLen; i++)
    {
        printf("Creating buffer %zu/%u with size %llu bytes\n", i, uniformBuffersLen - 1,
               (unsigned long long) bufferSize);

        createVkBuffer(pd, ld->device, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE,
                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                       &ubos->uniformBuffers[i], &ubos->uniformBuffersMemory[i]);

        printf("Buffer %zu created: handle=%p, memory=%p\n", i, (void*) (uintptr_t) ubos->uniformBuffers[i],
               (void*) (uintptr_t) ubos->uniformBuffersMemory[i]);

        vkMapMemory(ld->device, ubos->uniformBuffersMemory[i], 0, bufferSize, 0, &ubos->uniformBuffersMapped[i]);

        printf("Buffer %zu mapped at address: %p\n", i, ubos->uniformBuffersMapped[i]);
    }

    printf("flag createModelInstanceTransformationsUniformBuffersMapped\n");
    printf("=== UNIFORM BUFFER CREATION COMPLETE ===\n");
    return 1;
}

int createDeltaTimeUniformBuffers(PhysicalDevice* pd, LogicalDevice* ld, const uint32_t ubosLen,
                                  UniformBufferManager* ubos)
{

    assert(ubos);

    VkDeviceSize bufferSize = sizeof(UboDeltaTime);

    uint32_t uniformBuffersLen = ubosLen;
    if (uniformBuffersLen < 1)
    {
        uniformBuffersLen = 1;
    }

    ubos->uniformBuffers       = (VkBuffer*) malloc(sizeof(VkBuffer) * uniformBuffersLen);
    ubos->uniformBuffersMemory = (VkDeviceMemory*) malloc(sizeof(VkDeviceMemory) * uniformBuffersLen);
    ubos->uniformBuffersLen    = uniformBuffersLen;

    for (TRVZXS_SIZE i = 0; i < uniformBuffersLen; i++)
    {
        createVkBuffer(pd, ld->device, bufferSize,
                       VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_SHARING_MODE_EXCLUSIVE,
                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                       &ubos->uniformBuffers[i], &ubos->uniformBuffersMemory[i]);
    }

    printf("EN CREATEUNIFORMBUFFERS AL FINAL\n");
    return 1;
};
