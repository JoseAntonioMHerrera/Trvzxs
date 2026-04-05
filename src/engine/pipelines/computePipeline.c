#include "computePipeline.h"
#include "particle.h"

VkShaderModule* createComputeShaderModule(VkDevice device, char** code, size_t codeSize)
{
    VkShaderModuleCreateInfo createInfo = {
        .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = codeSize,
        .pCode    = (uint32_t*) *code,
    };

    VkShaderModule* shaderModule = malloc(sizeof(VkShaderModule));

    if (vkCreateShaderModule(device, &createInfo, VK_NULL_HANDLE, shaderModule) != VK_SUCCESS)
    {

        exit(0);
    }
    return shaderModule;
}

void createParticleComputePipeline(ComputePipeline* computeP, VkDevice device)
{

    // Init compute pipeline for graphics
    assert(computeP);
    char** computeParticle = (char**) malloc(sizeof(char*));

    size_t computeParticleSize = 0;

    readSPVBinaryFile(PARTICLE_COMPUTE, computeParticle, &computeParticleSize);
    // TODOcrear struct para spv, guarde datos y un hilo los lea y escriba en background
    assert(computeParticleSize > 0);

    computeP->computeShader = createComputeShaderModule(device, computeParticle, computeParticleSize);


    VkPipelineShaderStageCreateInfo computeShaderStageInfo = {
        .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage  = VK_SHADER_STAGE_COMPUTE_BIT,
        .module = (*computeP->computeShader),
        .pName  = "main",
        .pNext  = VK_NULL_HANDLE,
    };

    VkComputePipelineCreateInfo pipelineInfo = {
        .sType  = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .layout = *computeP->pipelineLayout,
        .stage  = computeShaderStageInfo,
    };
    computeP->pipeline = malloc(sizeof(*computeP->pipeline));
    if (vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, computeP->pipeline) != VK_SUCCESS)
    {
    }
}

void createOcclusionCullingComputePipeline(ComputePipeline* computeP, VkDevice device)
{

    assert(computeP);

    char** computeOclCulling = (char**) malloc(sizeof(char*));

    size_t computeGeometrySize = 0;

    readSPVBinaryFile(OCL_CULLING_COMPUTE, computeOclCulling, &computeGeometrySize);

    assert(computeGeometrySize > 0);

    computeP->computeShader = createComputeShaderModule(device, computeOclCulling, computeGeometrySize);

    VkPipelineShaderStageCreateInfo computeShaderStageInfo = {
        .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage  = VK_SHADER_STAGE_COMPUTE_BIT,
        .module = (*computeP->computeShader),
        .pName  = "main",
        .pNext  = VK_NULL_HANDLE,
    };

    VkComputePipelineCreateInfo pipelineInfo = {
        .sType  = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .layout = *computeP->pipelineLayout,
        .stage  = computeShaderStageInfo,
    };

    computeP->pipeline = malloc(sizeof(*computeP->pipeline));
    if (vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, computeP->pipeline) != VK_SUCCESS)
    {
    }
}


