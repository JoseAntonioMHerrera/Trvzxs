#include "uniformBuffer.h"
#include "camera.h"

MODEL_VIEW_PROJECTION* updateModelUniformBuffers(GraphicSwapchain* dsm, float deltaTime, Camera3D camera, SCENE* scene)
{
    // printf("=== UPDATING MODELS UNIFORM BUFFERS ===\n");
    // printf("Delta time: %.4f\n", deltaTime);

    static struct timespec startTime;
    static int             initialized = 0;
    struct timespec        currentTime;
    clock_gettime(CLOCK_MONOTONIC, &currentTime);

    // Calculate the difference in seconds
    float time = (currentTime.tv_sec - startTime.tv_sec) + (currentTime.tv_nsec - startTime.tv_nsec) / 1e9f;
    //  printf("Elapsed time: %.4f seconds\n", time);

    uint32_t transformationsCount = scene->modelsManager.modelInstanceLen;
    //    printf("Total transformations needed: %u (models: %u + lights: %u)\n", transformationsCount,
    //     scene->modelsManager.modelInstanceLen, scene->lightManager.lightInstanceCount);

    MODEL_VIEW_PROJECTION* pTransformations = malloc(sizeof(MODEL_VIEW_PROJECTION) * transformationsCount);
    //    printf("Allocated transformations array at %p (size: %zu bytes)\n", pTransformations,
    // sizeof(MODEL_VIEW_PROJECTION) * transformationsCount);
    uint32_t transformationsIdx = 0;
    //    printf("Processing %u model instances:\n", scene->modelsManager.modelInstanceLen);
    for (uint32_t modelIdx = 0; modelIdx < scene->modelsManager.modelLen; modelIdx++)
    {
        Model3D model = scene->modelsManager.pModel[modelIdx];
        for (uint32_t instanceIdx = 0; instanceIdx < model.instanceCount; instanceIdx++)
        {

            glm_mat4_copy(model.pModelInstance[instanceIdx].m, pTransformations[transformationsIdx].model);
            //          printf("    Model matrix copied from instance %u\n", transformationsIdx);

            // Print model matrix values
            /*       for (int i = 0; i < 4; i++)
                   {
                       for (int j = 0; j < 4; j++)
                       {
                           printf("Model[%d][%d] = %.6f ", i, j,
               pTransformations[transformationsIdx].model[i][j]);
                       }
                       printf("\n");
                   }*/

            // View matrix: camera at (2,2,2) looking at (0,0,0) with up vector (0,0,1)
            //            printf("    Setting up view matrix: eye(%.1f,%.1f,%.1f) center(%.1f,%.1f,%.1f)
            //            up(%.1f,%.1f,%.1f)\n",
            // camera.eye[0], camera.eye[1], camera.eye[2], camera.center[0], camera.center[1], camera.center[2],
            // camera.up[0], camera.up[1], camera.up[2]);

            vec4 view;
            glm_lookat(camera.eye, camera.center, camera.up, pTransformations[transformationsIdx].view);
            //            printf("    View matrix calculated\n");

            // Print view matrix values
            /*            for (int i = 0; i < 4; i++)
                        {
                            for (int j = 0; j < 4; j++)
                            {
                                printf("View[%d][%d] = %.6f ", i, j,
               pTransformations[transformationsIdx].view[i][j]);
                            }
                            printf("\n");
                        }
            */
            // Projection matrix: 45 degree FOV, aspect ratio based on swap chain extent
            float aspectRatio = dsm->currentExtent2D.width / (float) dsm->currentExtent2D.height;
            //    printf("    Projection setup: FOV=45°, aspect=%.3f (%ux%u), near=0.1, far=500.0\n",
            //    aspectRatio,
            // dsm->currentExtent2D.width, dsm->currentExtent2D.height);

            vec4 proj;
            glm_perspective(glm_rad(45.0f), aspectRatio, 2.0f, 500.0f, pTransformations[transformationsIdx].proj);
            //         printf("    Projection matrix calculated\n");

            // Adjust for Vulkan's coordinate system
            pTransformations[transformationsIdx].proj[1][1] *= -1;
            //        printf("    Vulkan Y-axis flip applied (proj[1][1] *= -1)\n");

            // Print projection matrix values
            /*      for (int i = 0; i < 4; i++)
                  {
                      for (int j = 0; j < 4; j++)
                      {
                          printf("Proj[%d][%d] = %.6f ", i, j, pTransformations[transformationsIdx].proj[i][j]);
                      }
                      printf("\n");
                  }
      */
            transformationsIdx++;
            //      printf("  Model instance %u transformation complete\n", transformationsIdx);
        }
    }

    // printf("All transformations processed successfully\n");
    //  printf("Returning transformations array: %p\n", pTransformations);
    //  printf("=== UNIFORM BUFFER UPDATE COMPLETE ===\n");
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
    // printf("=== UPDATING UNIFORM BUFFERS ===\n");
    //  printf("Delta time: %.4f\n", deltaTime);

    static struct timespec startTime;
    static int             initialized = 0;
    struct timespec        currentTime;
    clock_gettime(CLOCK_MONOTONIC, &currentTime);

    // Calculate the difference in seconds
    float time = (currentTime.tv_sec - startTime.tv_sec) + (currentTime.tv_nsec - startTime.tv_nsec) / 1e9f;
    //  printf("Elapsed time: %.4f seconds\n", time);

    uint32_t transformationsLen = scene->lightManager.lightInstanceCount;
    // printf("Total transformations needed: %u (lights: %u)\n", transformationsLen,
    //      scene->lightManager.lightInstanceCount);

    LightTransformations* pTransformations = malloc(sizeof(LightTransformations) * transformationsLen);
    //    printf("Allocated transformations array at %p (size: %zu bytes)\n", pTransformations,
    //       sizeof(LightTransformations) * transformationsLen);

    //    printf("Processing %u model instances:\n", scene->modelsManager.modelInstanceLen);
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
            //          printf("    Model matrix copied from instance %u\n", transformationsCount);

            // Print model matrix values
            /*         for (int i = 0; i < 4; i++)
                     {
                         for (int j = 0; j < 4; j++)
                         {
                             printf("Model[%d][%d] = %.6f ", i, j,
               pTransformations[transformationsCount].mvp.model[i][j]);
                         }
                         printf("\n");
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
            /*         printf("    Setting up view matrix: eye(%.1f,%.1f,%.1f) center(%.1f,%.1f,%.1f)
               up(%.1f,%.1f,%.1f)\n", camera.eye[0], camera.eye[1], camera.eye[2], camera.center[0],
               camera.center[1], camera.center[2], camera.up[0], camera.up[1], camera.up[2]); printf("    Setting
               up view matrix: eye(%.1f,%.1f,%.1f) center(%.1f,%.1f,%.1f) up(%.1f,%.1f,%.1f)\n",
                            camInstance->eye[0], camInstance->eye[1], camInstance->eye[2],
               camInstance->center[0], camInstance->center[1], camInstance->center[2], camInstance->up[0],
               camInstance->up[1], camInstance->up[2]);
             */
            vec4 view;
            glm_lookat(camera.eye, camera.center, camera.up, pTransformations[transformationsCount].mvp.view);
            // printf("    View matrix calculated\n");

            // Print view matrix values
            /*              for (int i = 0; i < 4; i++)
                          {
                              for (int j = 0; j < 4; j++)
                              {
                                  printf("View[%d][%d] = %.6f ", i, j,
               pTransformations[transformationsCount].mvp.view[i][j]);
                              }
                              printf("\n");
                          }
       */
            // Projection matrix: 45 degree FOV, aspect ratio based on swap chain extent
            float aspectRatio = dsm->currentExtent2D.width / (float) dsm->currentExtent2D.height;
            //          printf("    Projection setup: FOV=45°, aspect=%.3f (%ux%u), near=0.1, far=500.0\n",
            //          aspectRatio,
            //      dsm->currentExtent2D.width, dsm->currentExtent2D.height);

            vec4 proj;
            glm_perspective(glm_rad(45.0f), aspectRatio, 0.1f, 500.0f, pTransformations[transformationsCount].mvp.proj);
            //       printf("    Projection matrix calculated\n");

            // Adjust for Vulkan's coordinate system
            pTransformations[transformationsCount].mvp.proj[1][1] *= -1;
            //       printf("    Vulkan Y-axis flip applied (proj[1][1] *= -1)\n");
            mat4 proj_view;
            glm_mat4_mul(pTransformations[transformationsCount].mvp.proj,
                         pTransformations[transformationsCount].mvp.view, proj_view);

            glm_mat4_inv(proj_view, pTransformations[transformationsCount].mInverseVP);
            // Print projection matrix values
            /*       for (int i = 0; i < 4; i++)
                   {
                       for (int j = 0; j < 4; j++)
                       {
                           printf("Proj[%d][%d] = %.6f ", i, j,
               pTransformations[transformationsCount].mvp.proj[i][j]);
                       }
                       printf("\n");
                   }
*/
            //    printf("  Model instance %u transformation complete\n", transformationsCount);
            transformationsCount++;
        }
    }

    /*printf("All transformations processed successfully\n");
    printf("Returning transformations array: %p\n", pTransformations);
    printf("=== UNIFORM BUFFER UPDATE COMPLETE ===\n");*/
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

    ubos->uniformBuffers     = (VkBuffer*) malloc(sizeof(VkBuffer) * uniformBuffersLen);
    ubos->uniformAllocations = (VmaAllocation*) malloc(sizeof(VmaAllocation) * uniformBuffersLen);
    ubos->uniformBuffersLen  = uniformBuffersLen;

    for (TRVZXS_SIZE i = 0; i < uniformBuffersLen; i++)
    {
        createVkBufferMapped(ld->allocator, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE,
                             &ubos->uniformBuffers[i], &ubos->uniformAllocations[i], &ubos->uniformBuffersMapped[i]);
    }

    printf("=== MODEL UNIFORM BUFFER CREATION COMPLETE ===\n");
    return 1;
}

int createLightInstanceTransformationsUniformBuffersMapped(PhysicalDevice* pd, LogicalDevice* ld,
                                                           const uint32_t ubosLen, UniformBufferManager* ubos,
                                                           uint32_t lightInstanceCount)
{
    printf("=== CREATING LIGHT INSTANCE TRANSFORMATION UNIFORM BUFFERS ===\n");

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

    printf("=== LIGHT UNIFORM BUFFER CREATION COMPLETE ===\n");
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
        createVkBuffer(ld->allocator, bufferSize,
                       VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_SHARING_MODE_EXCLUSIVE,
                       VMA_BRIDGE_MEMORY_CPU_TO_GPU, &ubos->uniformBuffers[i], &ubos->uniformAllocations[i]);
    }

    printf("EN CREATEUNIFORMBUFFERS AL FINAL\n");
    return 1;
};
