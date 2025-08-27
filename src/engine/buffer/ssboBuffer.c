#include "ssboBuffer.h"
#include "primitives.h"
#include <math.h>
#include <time.h>
#include "graphicConfiguration.h"

float random_float() { return (float) rand() / (float) RAND_MAX; }

// Generate random float between min and max
float random_float_range(float min, float max) { return min + random_float() * (max - min); }

int random_int_range(int min, int max) { return min + rand() % (max - min + 1); }

int createParticleSSBOBuffer(PhysicalDevice* pd, LogicalDevice* ld, uint32_t ssbosLen, SSBOBufferManager* ssbos,
                             CommandPools* commandPool)
{

    assert(ssbos);
    srand(time(NULL));

    ssbos->buffersMapped =
        malloc(sizeof(*ssbos->buffersMapped) *
               ssbosLen); // TODO: ubosLen useless here, len is determined by maxframes from the graphics pipeline

    ssbos->buffers            = (VkBuffer*) malloc(sizeof(VkBuffer) * ssbosLen);
    ssbos->buffersMemory      = (VkDeviceMemory*) malloc(sizeof(VkDeviceMemory) * ssbosLen);
    ssbos->buffersLen         = ssbosLen;
    ssbos->readBackBufferSize = 1024;

    ParticleInstance* particles = malloc(sizeof(ParticleInstance) * 1024);

    // init particles position velocity and color
    // TODO: move particle inizialization data outside the function, here its only to create vk structs
    for (TRVZXS_SIZE particleIndex = 0; particleIndex < 1024; particleIndex++)
    {

        float r     = 0.25f * sqrt(random_float_range(0.0f, 1.0f));
        float theta = random_float_range(0.0f, 1.0f) * GLM_PI * 2;
        float x     = r * cos(theta) * WINDOW_HEIGHT / WINDOW_WIDTH;
        float y     = r * sin(theta);
        float z     = -0.6;

        glm_vec3_copy((vec3) {x, y, z}, particles[particleIndex].position.data);

        glm_normalize_to(particles[particleIndex].position.data, particles[particleIndex].velocity.data);

        particles[particleIndex].color = (GEOM_VERTEX_COLOR_RGBA) {
            r : (uint32_t) random_int_range(0, 255),
            g : (uint32_t) random_int_range(0, 180),
            b : (uint32_t) random_int_range(0, 61),
            a : 1.0f
        };

        if ((particleIndex % 1024) == 0)
        {
            printParticle(&particles[particleIndex]);
        }
    }

    printf("TRAS INICIALIZAR LAS PARTICULAS\n");

    VkDeviceSize particlesBufferSize = sizeof(ParticleInstance) * 1024;

    VkBuffer       stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    // We create the staging buffer
    createVkBuffer(pd, ld->device, particlesBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_SHARING_MODE_EXCLUSIVE,
                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer,
                   &stagingBufferMemory);
    printf("TRAS INICIALIZAR LAS PARTICULAS STAGING BUFFER\n");

    void* data;
    vkMapMemory(ld->device, stagingBufferMemory, 0, particlesBufferSize, 0, &data);
    memcpy(data, particles, (size_t) particlesBufferSize);
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
