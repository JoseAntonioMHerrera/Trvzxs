#include "computePipeline.h"
#include "particle.h"

 VkShaderModule* createComputeShaderModule(VkDevice device, char** code, size_t codeSize) {
    VkShaderModuleCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = codeSize,
        .pCode = (uint32_t*) *code,
    };

    VkShaderModule *shaderModule = malloc(sizeof(VkShaderModule));

    if (vkCreateShaderModule(device, &createInfo, VK_NULL_HANDLE, shaderModule) != VK_SUCCESS) {

        exit(0);
    }
    return shaderModule;
}

void createParticleComputePipeline(ComputePipeline* computeP, VkDevice device) {

    // Init compute pipeline for graphics
    assert(computeP);
    printf("CREATEPARTICLECOMPUTEPIPELINE\n");
    char** computeParticle = (char**) malloc(sizeof(char*));

    size_t computeParticleSize = 0;

    readSPVBinaryFile(PARTICLE_COMPUTE, computeParticle, &computeParticleSize);
    // TODOcrear struct para spv, guarde datos y un hilo los lea y escriba en background
    assert(computeParticleSize > 0);
    printf("CREATEPARTICLECOMPUTEPIPELINE 2\n");

    computeP->computeShader = createComputeShaderModule(device, computeParticle, computeParticleSize);

    printf("CREATEPARTICLECOMPUTEPIPELINE 3\n");

    VkPipelineShaderStageCreateInfo computeShaderStageInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_COMPUTE_BIT,
        .module = (*computeP->computeShader),
        .pName = "main",
        .pNext = VK_NULL_HANDLE,
    };
    printf("CREATEPARTICLECOMPUTEPIPELINE 4\n");

    VkComputePipelineCreateInfo pipelineInfo = {
        .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .layout = *computeP->pipelineLayout,
        .stage = computeShaderStageInfo,
    };
    printf("CREATEPARTICLECOMPUTEPIPELINE 44\n");
    computeP->pipeline = malloc(sizeof(*computeP->pipeline));
    if (vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, computeP->pipeline) != VK_SUCCESS) {
        printf("Error creating compute pipeline!");
    }
}

void debugParticles(ComputePipeline* computeP, VkDevice device) {
    // Map the memory
    void* data;
    vkMapMemory(device, computeP->deviceBuffer->ssbos->readBackBufferMemory, 0, VK_WHOLE_SIZE, 0, &data);

    // Cast to your particle struct
    GEOM_PARTICLE* particles = (GEOM_PARTICLE*)data;

    // Print first few particles
    for(int i = 0; i < 1024; i++) {
        printf("Particle %d: pos=(%.2f, %.2f, %.2f), vel=(%.2f, %.2f, %.2f)\n",
            i,
            particles[i].position.data[0],
            particles[i].position.data[1],
            particles[i].position.data[2],
            particles[i].velocity.data[0],
            particles[i].velocity.data[1],
            particles[i].velocity.data[2]
        );
    }

    // Unmap when done
    vkUnmapMemory(device, computeP->deviceBuffer->ssbos->readBackBufferMemory);
}

void recordParticleCommandBuffer(ComputePipeline* computeP, PhysicalDevice* pd, LogicalDevice* ld, VkPipelineLayout layout, VkDescriptorSet* descriptorSet, uint32_t imageIndex, float deltaTime){
    computeP->commandBuffer[imageIndex] = beginSingleTimeCommands(ld, computeP->commandPool->computeCommandPool);
    vkCmdBindPipeline(computeP->commandBuffer[imageIndex], VK_PIPELINE_BIND_POINT_COMPUTE, *computeP->pipeline);
    vkCmdBindDescriptorSets(computeP->commandBuffer[imageIndex], VK_PIPELINE_BIND_POINT_COMPUTE, layout, 0, 1, descriptorSet, 0, 0); // TODO: the read and update of descriptorSets can be synced too
    vkCmdUpdateBuffer(
        computeP->commandBuffer[imageIndex],
        *computeP->deviceBuffer->ubo->uniformBuffers,            // destination buffer
        0,                       // offset
        sizeof(float),           // size
        &deltaTime                // data
    );

    VkBufferMemoryBarrier barrier = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
        .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,  // Include shader writes
        .dstAccessMask = VK_ACCESS_SHADER_READ_BIT |  VK_ACCESS_TRANSFER_WRITE_BIT,   // Allow next transfer write
        .buffer = *computeP->deviceBuffer->ubo->uniformBuffers,
        .offset = 0,
        .size = sizeof(float)
    };

    vkCmdPipelineBarrier(
        computeP->commandBuffer[imageIndex],
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT,
        0,
        0, NULL,
        1, &barrier,
        0, NULL
    );
    vkCmdDispatch(computeP->commandBuffer[imageIndex], floor(1024 / 256), 1, 1);
    // Add barrier for SSBO writes
    VkBufferMemoryBarrier ssboBarrier = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
        .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,  // Remove vertex attribute read
        .srcQueueFamilyIndex = pd->queueFamilyIndices.computeFamily,    // Add these for queue
        .dstQueueFamilyIndex = pd->queueFamilyIndices.graphicsFamily,   // ownership transfer
        .buffer = computeP->deviceBuffer->ssbos->buffers[imageIndex],
        .offset = 0,
        .size = VK_WHOLE_SIZE
    };

    vkCmdPipelineBarrier(
        computeP->commandBuffer[imageIndex],
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,  // Remove vertex input bit here
        0,
        0, NULL,
        1, &ssboBarrier,
        0, NULL
    );

    // Copy to staging buffer
    VkBufferCopy copyRegion = {
        .srcOffset = 0,
        .dstOffset = 0,
        .size = sizeof(GEOM_PARTICLE) * 1024
    };

    vkCmdCopyBuffer(
        computeP->commandBuffer[imageIndex],
        computeP->deviceBuffer->ssbos->buffers[imageIndex],  // Source: your SSBO
        computeP->deviceBuffer->ssbos->readbackBuffer,  // Destination: staging buffer
        1,
        &copyRegion
    );
    debugParticles(computeP, ld->device);
    vkEndCommandBuffer(computeP->commandBuffer[imageIndex]);

}
