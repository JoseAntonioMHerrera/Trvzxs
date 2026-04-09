#include "uniformBuffer.h"
#include "buffer.h"
#include "camera.h"
#include "transform.h"
#include "transformNode.h"

void updateModelUniformBuffersDirect(void* mappedBuffer, SCENE* scene)
{
    ModelInstanceUBO* pInstances       = (ModelInstanceUBO*) mappedBuffer;
    uint32_t          transformationsIdx = 0;

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
            Model3DInstance* inst = &scene->modelsManager.pDenseModelInstances[model->denseBaseOffset + instanceIdx];

            if (inst->lifecycle != INSTANCE_ALIVE)
            {
                continue;
            }

            // Read resolved world matrix from transform hierarchy (resolved before UBO fill)
            TransformNode* tnode = &scene->transformManager.pTransforms[inst->transformIdx];

            // Skip if cascade-disabled by hierarchy resolve pass
            if (tnode->passFlags == TRANSFORM_PASS_DISABLED)
            {
                continue;
            }

            glm_mat4_copy(tnode->worldMatrix, pInstances[transformationsIdx].model);

            transformationsIdx++;
        }

        // Per-model live count — read by the draw loop
        model->liveInstanceCount = transformationsIdx - modelStartIdx;
    }

    // Update modelInstanceLen to the actual live count
    scene->modelsManager.modelInstanceLen = transformationsIdx;
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

void updateLightUniformBuffersDirect(void* mappedBuffer, float deltaTime, SCENE* scene)
{
    LightInstanceUBO* pInstances       = (LightInstanceUBO*) mappedBuffer;
    uint32_t          transformationsCount = 0;

    for (uint32_t lightIdx = 0; lightIdx < scene->lightManager.lightLen; lightIdx++)
    {
        LightVolume* light = &scene->lightManager.pLight[lightIdx];

        // Offset for this light = how many live transforms written so far
        scene->lightManager.instanceOffset.items[lightIdx] = transformationsCount;
        uint32_t lightStartIdx                             = transformationsCount;

        // Skip entire light if disabled or destroyed
        if (light->alive != LIGHT_ALIVE)
        {
            light->liveInstanceCount = 0;
            continue;
        }

        for (uint32_t instanceIdx = 0; instanceIdx < light->lightVolumeInstanceCount; instanceIdx++)
        {
            LightVolumeInstance* inst =
                &scene->lightManager.pDenseLightInstances[light->denseBaseOffset + instanceIdx];

            if (inst->lifecycle != LIGHT_INSTANCE_ALIVE)
            {
                continue;
            }

            // Read resolved world matrix from transform hierarchy
            TransformNode* tnode = &scene->transformManager.pTransforms[inst->transformIdx];

            // Skip if cascade-disabled by hierarchy resolve pass
            if (tnode->passFlags == TRANSFORM_PASS_DISABLED)
            {
                continue;
            }

            // Model matrix from resolved transform
            glm_mat4_copy(tnode->worldMatrix, pInstances[transformationsCount].model);

            // Extract position from world matrix column 3 (translation)
            vec3 worldPos;
            glm_vec3_copy(tnode->worldMatrix[3], worldPos);

            vec3 result;
            getRotatingPositionAroundLight(worldPos, result, deltaTime, 3);

            glm_vec3_copy(result, pInstances[transformationsCount].position);
            pInstances[transformationsCount].color = inst->color;
            pInstances[transformationsCount].scale = tnode->scale[0];

            transformationsCount++;
        }

        // Per-light live count — read by the draw loop
        light->liveInstanceCount = transformationsCount - lightStartIdx;
    }

    // Update lightInstanceCount to the actual live count
    scene->lightManager.lightInstanceCount = transformationsCount;
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

    VkDeviceSize bufferSize = sizeof(ModelInstanceUBO) * modelInstanceLen;

    uint32_t uniformBuffersLen = ubosLen;

    if (uniformBuffersLen < 1)
    {
        uniformBuffersLen = 1;
    }

    // Device-local SSBOs for fast GPU reads (descriptor binds these)
    ubos->uniformBuffers     = (VkBuffer*) malloc(sizeof(VkBuffer) * uniformBuffersLen);
    ubos->uniformAllocations = (VmaAllocation*) malloc(sizeof(VmaAllocation) * uniformBuffersLen);
    ubos->uniformBuffersLen  = uniformBuffersLen;

    for (TRVZXS_SIZE i = 0; i < uniformBuffersLen; i++)
    {
        createVkBuffer(ld->allocator, bufferSize,
                       VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                       VK_SHARING_MODE_EXCLUSIVE, VMA_BRIDGE_MEMORY_GPU_ONLY,
                       &ubos->uniformBuffers[i], &ubos->uniformAllocations[i]);
    }

    // Host-visible staging buffers for CPU writes (mapped pointers go here)
    ubos->stagingBuffers     = (VkBuffer*) malloc(sizeof(VkBuffer) * uniformBuffersLen);
    ubos->stagingAllocations = (VmaAllocation*) malloc(sizeof(VmaAllocation) * uniformBuffersLen);
    ubos->uniformBuffersMapped = malloc(sizeof(*ubos->uniformBuffersMapped) * uniformBuffersLen);
    ubos->stagingSize        = bufferSize;

    for (TRVZXS_SIZE i = 0; i < uniformBuffersLen; i++)
    {
        createVkBufferMapped(ld->allocator, bufferSize,
                             VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_SHARING_MODE_EXCLUSIVE,
                             &ubos->stagingBuffers[i], &ubos->stagingAllocations[i],
                             &ubos->uniformBuffersMapped[i]);
    }

    return 1;
}

int createLightInstanceTransformationsUniformBuffersMapped(PhysicalDevice* pd, LogicalDevice* ld,
                                                           const uint32_t ubosLen, UniformBufferManager* ubos,
                                                           uint32_t lightInstanceCount)
{

    assert(ubos);

    VkDeviceSize bufferSize = sizeof(LightInstanceUBO) * lightInstanceCount;

    uint32_t uniformBuffersLen = ubosLen;
    if (uniformBuffersLen < 1)
    {
        uniformBuffersLen = 1;
    }

    // Device-local SSBOs for fast GPU reads (descriptor binds these)
    ubos->uniformBuffers     = (VkBuffer*) malloc(sizeof(VkBuffer) * uniformBuffersLen);
    ubos->uniformAllocations = (VmaAllocation*) malloc(sizeof(VmaAllocation) * uniformBuffersLen);
    ubos->uniformBuffersLen  = uniformBuffersLen;

    for (TRVZXS_SIZE i = 0; i < uniformBuffersLen; i++)
    {
        createVkBuffer(ld->allocator, bufferSize,
                       VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                       VK_SHARING_MODE_EXCLUSIVE, VMA_BRIDGE_MEMORY_GPU_ONLY,
                       &ubos->uniformBuffers[i], &ubos->uniformAllocations[i]);
    }

    // Host-visible staging buffers for CPU writes (mapped pointers go here)
    ubos->stagingBuffers     = (VkBuffer*) malloc(sizeof(VkBuffer) * uniformBuffersLen);
    ubos->stagingAllocations = (VmaAllocation*) malloc(sizeof(VmaAllocation) * uniformBuffersLen);
    ubos->uniformBuffersMapped = malloc(sizeof(*ubos->uniformBuffersMapped) * uniformBuffersLen);
    ubos->stagingSize        = bufferSize;

    for (TRVZXS_SIZE i = 0; i < uniformBuffersLen; i++)
    {
        createVkBufferMapped(ld->allocator, bufferSize,
                             VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_SHARING_MODE_EXCLUSIVE,
                             &ubos->stagingBuffers[i], &ubos->stagingAllocations[i],
                             &ubos->uniformBuffersMapped[i]);
    }

    return 1;
}

int createFrameGlobalsUniformBuffersMapped(PhysicalDevice* pd, LogicalDevice* ld, const uint32_t ubosLen,
                                           UniformBufferManager* ubos)
{
    assert(ubos);

    VkDeviceSize bufferSize = sizeof(FrameGlobals);

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

void updateFrameGlobals(FrameGlobals* globals, GraphicSwapchain* dsm, Camera3D camera)
{
    // View matrix
    glm_lookat(camera.eye, camera.center, camera.up, globals->view);

    // Projection matrix
    float aspectRatio = dsm->currentExtent2D.width / (float) dsm->currentExtent2D.height;
    glm_perspective(glm_rad(45.0f), aspectRatio, 0.1f, 500.0f, globals->proj);
    globals->proj[1][1] *= -1; // Vulkan Y-flip

    // Precomputed viewProj
    glm_mat4_mul(globals->proj, globals->view, globals->viewProj);

    // Precomputed inverse viewProj (lights need this for reconstruction)
    glm_mat4_inv(globals->viewProj, globals->invViewProj);

    // Camera data
    globals->camera = camera;
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
