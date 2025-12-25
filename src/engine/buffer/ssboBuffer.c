#include "ssboBuffer.h"
#include "primitives.h"
#include <math.h>
#include <time.h>
#include "graphicConfiguration.h"
#include "gpuDrivenCommandGen.h"

float random_float() { return (float) rand() / (float) RAND_MAX; }

// Generate random float between min and max
float random_float_range(float min, float max) { return min + random_float() * (max - min); }

int random_int_range(int min, int max) { return min + rand() % (max - min + 1); }

int createParticleSSBOBuffer(PhysicalDevice* pd, LogicalDevice* ld, uint32_t ssbosLen, SSBOBufferManager* ssbos,
                             CommandPools* commandPool, SCENE* scene)
{

    assert(ssbos);
    srand(time(NULL));

    ssbos->buffersMapped =
        malloc(sizeof(*ssbos->buffersMapped) *
               ssbosLen); // TODO: ubosLen useless here, len is determined by maxframes from the graphics pipeline

    ssbos->buffers            = (VkBuffer*) malloc(sizeof(VkBuffer) * ssbosLen);
    ssbos->buffersMemory      = (VkDeviceMemory*) malloc(sizeof(VkDeviceMemory) * ssbosLen);
    ssbos->buffersLen         = ssbosLen;
    ssbos->readBackBufferSize = scene->particleManager.packedParticleInstanceLen;

    ParticleInstance* particles = malloc(sizeof(ParticleInstance) * scene->particleManager.packedParticleInstanceLen);

    printf("TRAS INICIALIZAR LAS PARTICULAS\n");

    VkDeviceSize particlesBufferSize = sizeof(ParticleInstance) * scene->particleManager.packedParticleInstanceLen;

    VkBuffer       stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    // We create the staging buffer
    createVkBuffer(pd, ld->device, particlesBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_SHARING_MODE_EXCLUSIVE,
                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer,
                   &stagingBufferMemory);
    printf("TRAS INICIALIZAR LAS PARTICULAS STAGING BUFFER\n");

    void* data;
    vkMapMemory(ld->device, stagingBufferMemory, 0, particlesBufferSize, 0, &data);
    memcpy(data, scene->particleManager.pParticleInstance, (size_t) particlesBufferSize);
    vkUnmapMemory(ld->device, stagingBufferMemory);

    printf("TRAS COPIAR PARTICLE DATA A LA GPU DESDE STAGING BUFFER\n");

    for (size_t ssboIndex = 0; ssboIndex < ssbosLen; ssboIndex++)
    {
        createVkBuffer(pd, ld->device, particlesBufferSize,
                       VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                           VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                       VK_SHARING_MODE_EXCLUSIVE, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &ssbos->buffers[ssboIndex],
                       &ssbos->buffersMemory[ssboIndex]);

        printf("TRAS CREAR EL VKBUFFER DE LAS PARTICULAS\n");
        // Copy data from the staging buffer (host) to the shader storage buffer (GPU)
        copyBuffer(ld, stagingBuffer, ssbos->buffers[ssboIndex], particlesBufferSize, commandPool->transferCommandPool,
                   *ld->queueManager.transferQueue);
    }

    // Clean up staging buffer resources
    vkDestroyBuffer(ld->device, stagingBuffer, NULL);
    vkFreeMemory(ld->device, stagingBufferMemory, NULL);

    // Clean up host memory
    free(particles);

    createVkBuffer(pd, ld->device, particlesBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_SHARING_MODE_EXCLUSIVE,
                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &ssbos->readbackBuffer,
                   &ssbos->readBackBufferMemory);

    return 1;
};

int createReadbackLightPass1SSBOBuffers(PhysicalDevice* pd, LogicalDevice* ld, uint32_t ssbosLen,
                                        SSBOBufferManager* ssbos, CommandPools* commandPool)
{

    assert(ssbos);

    ssbos->buffersMapped =
        malloc(sizeof(*ssbos->buffersMapped) *
               ssbosLen); // TODO: ubosLen useless here, len is determined by maxframes from the graphics pipeline

    ssbos->buffers            = (VkBuffer*) malloc(sizeof(VkBuffer) * ssbosLen);
    ssbos->buffersMemory      = (VkDeviceMemory*) malloc(sizeof(VkDeviceMemory) * ssbosLen);
    ssbos->buffersLen         = ssbosLen;
    ssbos->readBackBufferSize = WINDOW_WIDTH * WINDOW_HEIGHT;

    ReadBackBuffer* lightPixels = malloc(sizeof(ReadBackBuffer) * WINDOW_WIDTH * WINDOW_HEIGHT);

    VkDeviceSize lightPixelsBufferSize = sizeof(ReadBackBuffer) * WINDOW_WIDTH * WINDOW_HEIGHT;

    VkBuffer       stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    createVkBuffer(pd, ld->device, lightPixelsBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE,
                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &ssbos->readbackBuffer,
                   &ssbos->readBackBufferMemory);

    return 1;
};

/*uint32_t createOclCullingSSBOBuffer(PhysicalDevice* pd, LogicalDevice* ld, uint32_t ssbosLen, SSBOBufferManager*
ssbos, CommandPools* commandPool, SCENE* scene)
{
    assert(ssbos);
    srand(time(NULL));

    ssbos->buffersMapped =
        malloc(sizeof(*ssbos->buffersMapped) *
               ssbosLen); // TODO: ubosLen useless here, len is determined by maxframes from the graphics pipeline

    ssbos->buffers       = (VkBuffer*) malloc(sizeof(VkBuffer) * ssbosLen);
    ssbos->buffersMemory = (VkDeviceMemory*) malloc(sizeof(VkDeviceMemory) * ssbosLen);
    ssbos->buffersLen    = ssbosLen;

    RenderableObject* renderables         = malloc(sizeof(RenderableObject) * scene->packedRenderablesCount);
    VkDeviceSize      particlesBufferSize = sizeof(RenderableObject) * scene->packedRenderablesCount;

    VkBuffer       stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    createVkBuffer(pd, ld->device, particlesBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_SHARING_MODE_EXCLUSIVE,
                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer,
                   &stagingBufferMemory);

    void* data;
    vkMapMemory(ld->device, stagingBufferMemory, 0, particlesBufferSize, 0, &data);
    memcpy(data, scene->particleManager.pParticleInstance, (size_t) particlesBufferSize);
    vkUnmapMemory(ld->device, stagingBufferMemory);

    for (size_t ssboIndex = 0; ssboIndex < ssbosLen; ssboIndex++)
    {
        createVkBuffer(pd, ld->device, particlesBufferSize,
                       VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                           VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                       VK_SHARING_MODE_EXCLUSIVE, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &ssbos->buffers[ssboIndex],
                       &ssbos->buffersMemory[ssboIndex]);

        // Copy data from the staging buffer (host) to the shader storage buffer (GPU)
        copyBuffer(ld, stagingBuffer, ssbos->buffers[ssboIndex], particlesBufferSize, commandPool->transferCommandPool,
                   *ld->queueManager.transferQueue);
    }

    // Clean up staging buffer resources
    vkDestroyBuffer(ld->device, stagingBuffer, NULL);
    vkFreeMemory(ld->device, stagingBufferMemory, NULL);

    return 0;
};*/
