#include "uniformBuffer.h"
#include "camera.h"

MODEL_VIEW_PROJECTION* updateModelUniformBuffers(GraphicSwapchain* dsm, float deltaTime, Camera3D camera, SCENE* scene)
{

    static struct timespec startTime;
    static int             initialized = 0;
    struct timespec        currentTime;
    clock_gettime(CLOCK_MONOTONIC, &currentTime);

    // Calculate the difference in seconds
    float time = (currentTime.tv_sec - startTime.tv_sec) + (currentTime.tv_nsec - startTime.tv_nsec) / 1e9f;

    // Allocate for full capacity (sum of all instanceCount) — compaction fills only the live portion.
    // modelInstanceLen cannot be used here because the fill loop overwrites it with the live count,
    // which would shrink the allocation on the next frame and cause a buffer overflow if instances
    // are re-enabled.
    uint32_t totalCapacity = 0;
    for (uint32_t i = 0; i < scene->modelsManager.modelLen; i++)
    {
        totalCapacity += scene->modelsManager.pModel[i].instanceCount;
    }

    MODEL_VIEW_PROJECTION* pTransformations = malloc(sizeof(MODEL_VIEW_PROJECTION) * totalCapacity);
    uint32_t               transformationsIdx = 0;

    for (uint32_t modelIdx = 0; modelIdx < scene->modelsManager.modelLen; modelIdx++)
    {
        Model3D* model = &scene->modelsManager.pModel[modelIdx];

        // Offset for this model = how many live transforms written so far
        scene->modelsManager.instanceOffset.items[modelIdx] = transformationsIdx;
        uint32_t modelStartIdx                              = transformationsIdx;

        // Skip entire model if disabled or destroyed — no instance iteration needed
        if (model->alive != 1)
        {
            model->liveInstanceCount = 0;
            continue;
        }

        for (uint32_t instanceIdx = 0; instanceIdx < model->instanceCount; instanceIdx++)
        {
            if (model->pModelInstance[instanceIdx].alive != 1)
            {
                continue;
            }

            glm_mat4_copy(model->pModelInstance[instanceIdx].m, pTransformations[transformationsIdx].model);

            vec4 view;
            glm_lookat(camera.eye, camera.center, camera.up, pTransformations[transformationsIdx].view);

            float aspectRatio = dsm->currentExtent2D.width / (float) dsm->currentExtent2D.height;
            glm_perspective(glm_rad(45.0f), aspectRatio, 2.0f, 500.0f, pTransformations[transformationsIdx].proj);
            pTransformations[transformationsIdx].proj[1][1] *= -1;

            transformationsIdx++;
        }

        // Per-model live count — read by the draw loop
        model->liveInstanceCount = transformationsIdx - modelStartIdx;
    }

    // Update modelInstanceLen to the actual live count for the memcpy
    scene->modelsManager.modelInstanceLen = transformationsIdx;

    return pTransformations;
}

void getRotatingPositionAroundLight(vec3 lightPos, vec3 result, float deltaTime, float radius)
{
    static float accumulatedAngle = 0.0f; // Maintains rotation state

    // Fixed rotation speed (radians per second)
    float rotationSpeed = 2.0f; // Adjust as needed

    // Update angle with deltaTime
    accumulatedAngle += rotationSpeed * deltaTime;

    // Keep angle in reasonable range
    if (accumulatedAngle > 2.0f * M_PI)
    {
        accumulatedAngle -= 2.0f * M_PI;
    }

    // Rotate around Y-axis (vertical axis) - fixed direction
    result[0] = lightPos[0] + cosf(accumulatedAngle) * radius; // X
    result[1] = lightPos[1];                                   // Y (unchanged)
    result[2] = lightPos[2] + sinf(accumulatedAngle) * radius; // Z
}

LightTransformations* updateLightUniformBuffers(GraphicSwapchain* dsm, float deltaTime, Camera3D camera, SCENE* scene)
{

    static struct timespec startTime;
    static int             initialized = 0;
    struct timespec        currentTime;
    clock_gettime(CLOCK_MONOTONIC, &currentTime);

    // Calculate the difference in seconds
    float time = (currentTime.tv_sec - startTime.tv_sec) + (currentTime.tv_nsec - startTime.tv_nsec) / 1e9f;

    uint32_t transformationsLen = scene->lightManager.lightInstanceCount;
    //      scene->lightManager.lightInstanceCount);

    LightTransformations* pTransformations = malloc(sizeof(LightTransformations) * transformationsLen);
    //       sizeof(LightTransformations) * transformationsLen);

    uint32_t transformationsCount = 0;
    for (uint32_t lightIdx = 0; lightIdx < scene->lightManager.lightLen; lightIdx++)
    {
        LightVolume light = scene->lightManager.pLight[lightIdx];
        for (uint32_t instanceIdx = 0; instanceIdx < light.lightVolumeInstanceCount; instanceIdx++)
        {
            mat4 auxModelInstance;
            glm_mat4_identity(auxModelInstance);

            vec3 scale = {light.pLightVolumeInstance[instanceIdx].scale, light.pLightVolumeInstance[instanceIdx].scale,
                          light.pLightVolumeInstance[instanceIdx].scale};
            glm_translate_make(auxModelInstance, light.pLightVolumeInstance[instanceIdx].position);
            glm_scale(auxModelInstance, scale);

            // Model matrix: rotate around Z-axis
            glm_mat4_copy(auxModelInstance, pTransformations[transformationsCount].mvp.model);

            // Print model matrix values
            /*         for (int i = 0; i < 4; i++)
                     {
                         for (int j = 0; j < 4; j++)
                         {
               pTransformations[transformationsCount].mvp.model[i][j]);
                         }
                     }
         */
            // View matrix: camera at (2,2,2) looking at (0,0,0) with up vector (0,0,1)

            Camera3D* camInstance = (Camera3D*) malloc(sizeof(pTransformations[transformationsCount].camera));
            glm_vec2_copy(camera.screenSize, camInstance->screenSize);
            glm_vec3_copy(camera.eye, camInstance->eye);
            glm_vec3_copy(camera.center, camInstance->center);
            glm_vec3_copy(camera.up, camInstance->up);
            pTransformations[transformationsCount].camera = *camInstance;

            vec3 result;
            getRotatingPositionAroundLight(light.pLightVolumeInstance[instanceIdx].position, result, deltaTime, 3);

            glm_vec3_copy(result, pTransformations[transformationsCount].data.position);
            pTransformations[transformationsCount].data.color = light.pLightVolumeInstance[instanceIdx].color;
            pTransformations[transformationsCount].data.scale = light.pLightVolumeInstance[instanceIdx].scale;
            vec4 view;
            glm_lookat(camera.eye, camera.center, camera.up, pTransformations[transformationsCount].mvp.view);

            // Print view matrix values
            /*              for (int i = 0; i < 4; i++)
                          {
                              for (int j = 0; j < 4; j++)
                              {
               pTransformations[transformationsCount].mvp.view[i][j]);
                              }
                          }
       */
            // Projection matrix: 45 degree FOV, aspect ratio based on swap chain extent
            float aspectRatio = dsm->currentExtent2D.width / (float) dsm->currentExtent2D.height;
            //          aspectRatio,
            //      dsm->currentExtent2D.width, dsm->currentExtent2D.height);

            vec4 proj;
            glm_perspective(glm_rad(45.0f), aspectRatio, 0.1f, 500.0f, pTransformations[transformationsCount].mvp.proj);

            // Adjust for Vulkan's coordinate system
            pTransformations[transformationsCount].mvp.proj[1][1] *= -1;
            mat4 proj_view;
            glm_mat4_mul(pTransformations[transformationsCount].mvp.proj,
                         pTransformations[transformationsCount].mvp.view, proj_view);

            glm_mat4_inv(proj_view, pTransformations[transformationsCount].mInverseVP);
            // Print projection matrix values
            /*       for (int i = 0; i < 4; i++)
                   {
                       for (int j = 0; j < 4; j++)
                       {
               pTransformations[transformationsCount].mvp.proj[i][j]);
                       }
                   }
*/
            transformationsCount++;
        }
    }

    return pTransformations;
}

void freeMappedUniformBufferLocalMemory(mat4** instancedTranformations)
{
    free(*instancedTranformations);
    *instancedTranformations = 0;
}

int createModelInstanceTransformationsUniformBuffersMapped(PhysicalDevice* pd, LogicalDevice* ld,
                                                           const uint32_t ubosLen, UniformBufferManager* ubos,
                                                           uint32_t modelInstanceLen)
{

    assert(ubos);

    VkDeviceSize bufferSize = sizeof(MODEL_VIEW_PROJECTION) * modelInstanceLen;

    ubos->uniformBuffersMapped = malloc(sizeof(*ubos->uniformBuffersMapped) * ubosLen);

    uint32_t uniformBuffersLen = ubosLen;

    if (uniformBuffersLen < 1)
    {
        uniformBuffersLen = 1;
    }

    ubos->uniformBuffers     = (VkBuffer*) malloc(sizeof(VkBuffer) * uniformBuffersLen);
    ubos->uniformAllocations = (VmaAllocation*) malloc(sizeof(VmaAllocation) * uniformBuffersLen);
    ubos->uniformBuffersLen  = uniformBuffersLen;

    for (TRVZXS_SIZE i = 0; i < uniformBuffersLen; i++)
    {
        createVkBufferMapped(ld->allocator, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE,
                             &ubos->uniformBuffers[i], &ubos->uniformAllocations[i], &ubos->uniformBuffersMapped[i]);
    }

    return 1;
}

int createLightInstanceTransformationsUniformBuffersMapped(PhysicalDevice* pd, LogicalDevice* ld,
                                                           const uint32_t ubosLen, UniformBufferManager* ubos,
                                                           uint32_t lightInstanceCount)
{

    assert(ubos);

    VkDeviceSize bufferSize = sizeof(LightTransformations) * lightInstanceCount;

    ubos->uniformBuffersMapped = malloc(sizeof(*ubos->uniformBuffersMapped) * ubosLen);

    uint32_t uniformBuffersLen = ubosLen;
    if (uniformBuffersLen < 1)
    {
        uniformBuffersLen = 1;
    }

    ubos->uniformBuffers     = (VkBuffer*) malloc(sizeof(VkBuffer) * uniformBuffersLen);
    ubos->uniformAllocations = (VmaAllocation*) malloc(sizeof(VmaAllocation) * uniformBuffersLen);
    ubos->uniformBuffersLen  = uniformBuffersLen;

    for (TRVZXS_SIZE i = 0; i < uniformBuffersLen; i++)
    {
        createVkBufferMapped(ld->allocator, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE,
                             &ubos->uniformBuffers[i], &ubos->uniformAllocations[i], &ubos->uniformBuffersMapped[i]);
    }

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

    ubos->uniformBuffers     = (VkBuffer*) malloc(sizeof(VkBuffer) * uniformBuffersLen);
    ubos->uniformAllocations = (VmaAllocation*) malloc(sizeof(VmaAllocation) * uniformBuffersLen);
    ubos->uniformBuffersLen  = uniformBuffersLen;

    for (TRVZXS_SIZE i = 0; i < uniformBuffersLen; i++)
    {
        createVkBuffer(ld->allocator, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                       VK_SHARING_MODE_EXCLUSIVE, VMA_BRIDGE_MEMORY_CPU_TO_GPU, &ubos->uniformBuffers[i],
                       &ubos->uniformAllocations[i]);
    }

    return 1;
};
