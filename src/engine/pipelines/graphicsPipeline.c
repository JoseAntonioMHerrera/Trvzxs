#include "graphicsPipeline.h"
#include "light.h"
#include "particle.h"

#define TRIANGLE_VERT_SHADER "shaders/bin/triangle.vert.spv\0"
#define TRIANGLE_FRAG_SHADER "shaders/bin/triangle.frag.spv\0"

void createSyncObjects(GraphicPipelineToSurface* gd, VkDevice* device)
{
    VkSemaphoreCreateInfo semaphoreInfo = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    };

    assert(gd->commandLen >= 2);
    gd->imgViewSemaphore = malloc(sizeof(VkSemaphore) * gd->commandLen);
    gd->renderEndFlag    = malloc(sizeof(VkSemaphore) * gd->commandLen);

    assert(gd->renderEndFlag);
    assert(gd->imgViewSemaphore);

    for (uint32_t i = 0; i < gd->commandLen; i++)
    {
        if (vkCreateSemaphore(*device, &semaphoreInfo, NULL, &gd->imgViewSemaphore[i]) != VK_SUCCESS ||
            vkCreateSemaphore(*device, &semaphoreInfo, NULL, &gd->renderEndFlag[i]) != VK_SUCCESS)
        {
        }
    }
}

void fillParticleVertexBindingDescriptions(VkVertexInputBindingDescription** objectIBD, VkVertexInputRate inputRate,
                                           size_t stride)
{

    *objectIBD              = malloc(sizeof(VkVertexInputBindingDescription));
    (*objectIBD)->binding   = 0;
    (*objectIBD)->stride    = stride;
    (*objectIBD)->inputRate = inputRate;
}

void fillParticleVertexInputAttributeDescription(VkVertexInputAttributeDescription** objectsIAD, uint32_t viadLen)
{ // MOVE  object input attribute description to each primitive
    *objectsIAD = (VkVertexInputAttributeDescription*) malloc(sizeof(VkVertexInputAttributeDescription) * viadLen);
    // GEOM_OBJECT coordinates

    (*objectsIAD)[0].binding  = 0;
    (*objectsIAD)[0].location = 0;
    (*objectsIAD)[0].format   = VK_FORMAT_R32G32B32_SFLOAT;
    (*objectsIAD)[0].offset   = offsetof(TrzVertex, point);

    // GEOM_OBJECT colors
    (*objectsIAD)[1].binding  = 0;
    (*objectsIAD)[1].location = 1;
    (*objectsIAD)[1].format   = VK_FORMAT_R32G32B32_SFLOAT;
    (*objectsIAD)[1].offset   = offsetof(TrzVertex, color);
    // GEOM_OBJECT colors
    (*objectsIAD)[2].binding  = 0;
    (*objectsIAD)[2].location = 2;
    (*objectsIAD)[2].format   = VK_FORMAT_R32G32B32_SFLOAT;
    (*objectsIAD)[2].offset   = offsetof(TrzVertex, bary);
}


VkShaderModule* createShaderModule(VkDevice device, char** code, size_t codeSize)
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

void fillModelVertexBindingDescriptions(VkVertexInputBindingDescription** objectIBD, VkVertexInputRate inputRate,
                                        size_t stride)
{

    *objectIBD              = malloc(sizeof(VkVertexInputBindingDescription));
    (*objectIBD)->binding   = 0;
    (*objectIBD)->stride    = stride;
    (*objectIBD)->inputRate = inputRate;
}

void fillModelVertexInputAttributeDescription(VkVertexInputAttributeDescription** objectsIAD, uint32_t viadLen)
{ // MOVE  object input attribute description to each primitive
    *objectsIAD = (VkVertexInputAttributeDescription*) malloc(sizeof(VkVertexInputAttributeDescription) * viadLen);
    // GEOM_OBJECT coordinates

    (*objectsIAD)[0].binding  = 0;
    (*objectsIAD)[0].location = 0;
    (*objectsIAD)[0].format   = VK_FORMAT_R32G32B32_SFLOAT;
    (*objectsIAD)[0].offset   = offsetof(GEOM_VERTEX, point);

    // GEOM_OBJECT colors
    (*objectsIAD)[1].binding  = 0;
    (*objectsIAD)[1].location = 1;
    (*objectsIAD)[1].format   = VK_FORMAT_R32G32B32_SFLOAT;
    (*objectsIAD)[1].offset   = offsetof(GEOM_VERTEX, color);

    // GEOM_OBJECT colors
    (*objectsIAD)[2].binding  = 0;
    (*objectsIAD)[2].location = 2;
    (*objectsIAD)[2].format   = VK_FORMAT_R32G32B32_SFLOAT;
    (*objectsIAD)[2].offset   = offsetof(GEOM_VERTEX, textCoords);

    // GEOM_OBJECT colors
    (*objectsIAD)[3].binding  = 0;
    (*objectsIAD)[3].location = 3;
    (*objectsIAD)[3].format   = VK_FORMAT_R32G32B32_SFLOAT;
    (*objectsIAD)[3].offset   = offsetof(GEOM_VERTEX, normal);
}

void fillDepthPassVertexBindingDescriptions(VkVertexInputBindingDescription** objectIBD, VkVertexInputRate inputRate,
                                            size_t stride)
{

    *objectIBD              = malloc(sizeof(VkVertexInputBindingDescription));
    (*objectIBD)->binding   = 0;
    (*objectIBD)->stride    = stride;
    (*objectIBD)->inputRate = inputRate;
}

void fillDepthPassVertexInputAttributeDescription(VkVertexInputAttributeDescription** objectsIAD, uint32_t viadLen)
{ // MOVE  object input attribute description to each primitive
    *objectsIAD = (VkVertexInputAttributeDescription*) malloc(sizeof(VkVertexInputAttributeDescription) * viadLen);
    // GEOM_OBJECT coordinates

    (*objectsIAD)[0].binding  = 0;
    (*objectsIAD)[0].location = 0;
    (*objectsIAD)[0].format   = VK_FORMAT_R32G32B32_SFLOAT;
    (*objectsIAD)[0].offset   = offsetof(GEOM_VERTEX, point);
}

void fillLightVolumeBindingDescriptions(VkVertexInputBindingDescription** objectIBD, VkVertexInputRate inputRate,
                                        size_t stride)
{

    *objectIBD              = malloc(sizeof(VkVertexInputBindingDescription));
    (*objectIBD)->binding   = 0;
    (*objectIBD)->stride    = stride;
    (*objectIBD)->inputRate = inputRate;
}

void fillLightVolumeVertexInputAttributeDescription(VkVertexInputAttributeDescription** objectsIAD, uint32_t viadLen)
{ // MOVE  object input attribute description to each primitive
    *objectsIAD = (VkVertexInputAttributeDescription*) malloc(sizeof(VkVertexInputAttributeDescription) * viadLen);
    // GEOM_OBJECT coordinates

    (*objectsIAD)[0].binding  = 0;
    (*objectsIAD)[0].location = 0;
    (*objectsIAD)[0].format   = VK_FORMAT_R32G32B32_SFLOAT;
    (*objectsIAD)[0].offset   = offsetof(GEOM_VERTEX, point);

    (*objectsIAD)[1].binding  = 0;
    (*objectsIAD)[1].location = 1;
    (*objectsIAD)[1].format   = VK_FORMAT_R32G32B32_SFLOAT;
    (*objectsIAD)[1].offset   = offsetof(GEOM_VERTEX, bary);
}

/*
 void createParticleRenderPass(PhysicalDevice *pd, GraphicSwapchain *dsm, VkDevice device, GraphicPipelineToSurface
*gdp) {
    //TODO make abstractions for resources attachtments and resolvers
    VkAttachmentReference depthAttachmentRef = {
        .attachment = 1,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };

    VkAttachmentDescription depthAttachment = {
        .format = (findDepthImageFormat(pd->gpu)),
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        .samples = VK_SAMPLE_COUNT_1_BIT,
    };

    VkAttachmentDescription colorAttachment = {
        .format = (dsm->currentSurfaceFormat)->format,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .samples = VK_SAMPLE_COUNT_1_BIT,
    };
        VkAttachmentReference colorAttachmentRef = {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };

    VkSubpassDescription subpass = {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachmentRef,
        .pDepthStencilAttachment = &depthAttachmentRef,
        .pResolveAttachments =  NULL,
    };


    // TODO MANAGE SUBPASS TREE

    dsm->subpassDependency = malloc(sizeof(VkSubpassDependency)); //TODO this subpassDependency doesnt need to be  a
pointer dsm->subpassDependency->srcSubpass = VK_SUBPASS_EXTERNAL; dsm->subpassDependency->dstSubpass = 0;
    dsm->subpassDependency->srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT; dsm->subpassDependency->srcAccessMask = 0;
    dsm->subpassDependency->dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT; dsm->subpassDependency->dstAccessMask =
VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
dsm->subpassDependency->dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT; VkAttachmentDescription attatchments[2] =
{colorAttachment, depthAttachment}; VkRenderPassCreateInfo renderPassInfo = { .sType =
VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO, .attachmentCount = 2, .pAttachments = attatchments, .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = dsm->subpassDependency,
    };

    gdp->renderPass = malloc(sizeof(*gdp->renderPass));
    if (vkCreateRenderPass(device, &renderPassInfo, NULL, gdp->renderPass) != VK_SUCCESS) {

        exit(0);
    }


}*/

void createModelsGraphicPipeline(GraphicPipelineToSurface* gdp, DisplayManager* dm, VkDevice* device)
{
    char** triangleVert = (char**) malloc(sizeof(char*));
    char** triangleFrag = (char**) malloc(sizeof(char*));

    size_t triangleVertSize = 0;
    size_t triangleFragSize = 0;

    readSPVBinaryFile(TRIANGLE_VERT_SHADER, triangleVert, &triangleVertSize);
    readSPVBinaryFile(TRIANGLE_FRAG_SHADER, triangleFrag, &triangleFragSize);
    // TODOcrear struct para spv, guarde datos y un hilo los lea y escriba en background
    assert(triangleFragSize > 0);
    assert(triangleVertSize > 0);

    gdp->vertShader = createShaderModule(*device, triangleVert, triangleVertSize);
    gdp->fragShader = createShaderModule(*device, triangleFrag, triangleFragSize);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo = {
        .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage  = VK_SHADER_STAGE_VERTEX_BIT,
        .module = (*gdp->vertShader),
        .pName  = "main",
        .pNext  = VK_NULL_HANDLE,
    };

    VkPipelineShaderStageCreateInfo fragShaderStageInfo = {
        .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage  = VK_SHADER_STAGE_FRAGMENT_BIT,
        .module = (*gdp->fragShader),
        .pName  = "main",
        .pNext  = VK_NULL_HANDLE,
    };

    VkPipelineShaderStageCreateInfo shaderStages[2];
    shaderStages[0] = fragShaderStageInfo;
    shaderStages[1] = vertShaderStageInfo;

    VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

    uint32_t dynamicStateSize = 2;

    VkPipelineDynamicStateCreateInfo dynamicState = {
        .sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = dynamicStateSize,
        .pDynamicStates    = dynamicStates,
        .pNext             = VK_NULL_HANDLE,
    };

    VkPipelineDepthStencilStateCreateInfo depthStencil = {
        .sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable       = VK_TRUE,
        .depthWriteEnable      = VK_TRUE,
        .depthCompareOp        = VK_COMPARE_OP_LESS,
        .depthBoundsTestEnable = VK_FALSE,
        .minDepthBounds        = 0.0f, // Optional
        .maxDepthBounds        = 1.0f, // Optional
        .stencilTestEnable     = VK_FALSE,
        .front                 = {}, // Optional
        .back                  = {}, // Optional
    };

    // MODELS VERTEX INPUT
    gdp->objectIADLen = 4;
    fillModelVertexInputAttributeDescription(&gdp->objectIAD, gdp->objectIADLen);
    gdp->objectIBDLen = 1;
    fillModelVertexBindingDescriptions(&gdp->objectIBD, VK_VERTEX_INPUT_RATE_VERTEX, sizeof(GEOM_VERTEX));

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {
        .sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount   = gdp->objectIBDLen,
        .pVertexBindingDescriptions      = gdp->objectIBD,
        .vertexAttributeDescriptionCount = gdp->objectIADLen,
        .pVertexAttributeDescriptions    = gdp->objectIAD,
        .pNext                           = VK_NULL_HANDLE,
    };

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE,
        .pNext                  = VK_NULL_HANDLE,
    };

    VkViewport viewport = {
        .x        = 0.0f,
        .y        = 0.0f,
        .width    = (float) dm->graphicPipeline->swapchain->currentExtent2D.width,
        .height   = (float) dm->graphicPipeline->swapchain->currentExtent2D.height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };

    VkRect2D scissor = {
        .offset = {0, 0},
        .extent = dm->graphicPipeline->swapchain->currentExtent2D,
    };

    VkPipelineViewportStateCreateInfo viewportState = {
        .sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .pViewports    = &viewport,
        .scissorCount  = 1,
        .pScissors     = &scissor,
        .pNext         = VK_NULL_HANDLE,
    };

    VkPipelineRasterizationStateCreateInfo rasterizer = {
        .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable        = VK_FALSE,
        .polygonMode             = VK_POLYGON_MODE_FILL,
        .lineWidth               = 1.0f,
        .cullMode                = VK_CULL_MODE_BACK_BIT,
        .frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .depthBiasEnable         = VK_FALSE,
        .depthBiasConstantFactor = 0.0f, // Optional
        .depthBiasClamp          = 0.0f, // Optional
        .depthBiasSlopeFactor    = 0.0f, // Optional
        .pNext                   = VK_NULL_HANDLE,
    };

    VkPipelineMultisampleStateCreateInfo multisampling = {
        .sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .sampleShadingEnable   = VK_FALSE,
        .rasterizationSamples  = dm->graphicPipeline->swapchain->msaaColorRes->numSamples,
        .minSampleShading      = 1.0f,     // Optional
        .pSampleMask           = NULL,     // Optional
        .alphaToCoverageEnable = VK_FALSE, // Optional
        .alphaToOneEnable      = VK_FALSE, // Optional
        .pNext                 = VK_NULL_HANDLE,
    };

    VkPipelineColorBlendAttachmentState colorBlendAttachment[2] = {
        {
            .blendEnable    = VK_FALSE,
            .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
                              VK_COLOR_COMPONENT_A_BIT,
        },
        {
            .blendEnable    = VK_FALSE,
            .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
                              VK_COLOR_COMPONENT_A_BIT,
        }};

    uint32_t                            colorBlendAttachmentCount = 2;
    VkPipelineColorBlendStateCreateInfo colorBlending             = {
                    .sType             = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
                    .logicOpEnable     = VK_FALSE,
                    .logicOp           = VK_LOGIC_OP_COPY, // Optional
                    .attachmentCount   = colorBlendAttachmentCount,
                    .pAttachments      = colorBlendAttachment,
                    .blendConstants[0] = 0.3f, // Optional
                    .blendConstants[1] = 0.4f, // Optional
                    .blendConstants[2] = 0.5f, // Optional
                    .blendConstants[3] = 0.0f, // Optional
                    .pNext             = VK_NULL_HANDLE,
    };

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
        .sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = gdp->maxFrames, // Optional
        .pSetLayouts    = (VkDescriptorSetLayout[]) {gdp->dsl, gdp->dsl, gdp->dsl},
        .pNext          = VK_NULL_HANDLE,
    };

    gdp->pipelineLayout = malloc(sizeof(*gdp->pipelineLayout));
    if (vkCreatePipelineLayout(*device, &pipelineLayoutInfo, NULL, gdp->pipelineLayout) != VK_SUCCESS)
    {

        exit(1);
    }

    // createRenderPass(dm->swapchain,*device,gdp);

    VkGraphicsPipelineCreateInfo pipelineInfo = {
        .sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount          = 2,
        .pStages             = &shaderStages[0],
        .pVertexInputState   = &vertexInputInfo,
        .pInputAssemblyState = &inputAssembly,
        .pViewportState      = &viewportState,
        .pRasterizationState = &rasterizer,
        .pMultisampleState   = &multisampling,
        .pDepthStencilState  = NULL, // Optional
        .pColorBlendState    = &colorBlending,
        .pDynamicState       = &dynamicState,
        .layout              = *gdp->pipelineLayout,
        .renderPass          = *gdp->renderPass,
        .subpass             = 0,
        .pDepthStencilState  = &depthStencil,
        .basePipelineHandle  = VK_NULL_HANDLE, // Optional
        .basePipelineIndex   = -1,             // Optional
        .pNext               = VK_NULL_HANDLE,
    };
    gdp->pipeline = malloc(sizeof(*gdp->pipeline));
    if (vkCreateGraphicsPipelines(*device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, gdp->pipeline) != VK_SUCCESS)
    {

        exit(1);
    }
}

void createParticleGraphicPipeline(GraphicPipelineToSurface* gdp, DisplayManager* dm, VkDevice* device)
{
    char** particleVert = (char**) malloc(sizeof(char*));
    char** particleFrag = (char**) malloc(sizeof(char*));

    size_t particleVertSize = 0;
    size_t particleFragSize = 0;

    readSPVBinaryFile(PARTICLE_VERT_SHADER, particleVert, &particleVertSize);
    readSPVBinaryFile(PARTICLE_FRAG_SHADER, particleFrag, &particleFragSize);
    // TODOcrear struct para spv, guarde datos y un hilo los lea y escriba en background
    assert(particleVertSize > 0);
    assert(particleFragSize > 0);

    gdp->vertShader = createShaderModule(*device, particleVert, particleVertSize);
    gdp->fragShader = createShaderModule(*device, particleFrag, particleFragSize);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo = {
        .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage  = VK_SHADER_STAGE_VERTEX_BIT,
        .module = (*gdp->vertShader),
        .pName  = "main",
        .pNext  = VK_NULL_HANDLE,
    };

    VkPipelineShaderStageCreateInfo fragShaderStageInfo = {
        .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage  = VK_SHADER_STAGE_FRAGMENT_BIT,
        .module = (*gdp->fragShader),
        .pName  = "main",
        .pNext  = VK_NULL_HANDLE,
    };

    VkPipelineShaderStageCreateInfo shaderStages[2];
    shaderStages[0] = fragShaderStageInfo;
    shaderStages[1] = vertShaderStageInfo;

    VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

    uint32_t dynamicStateSize = 2;

    VkPipelineDynamicStateCreateInfo dynamicState = {
        .sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = dynamicStateSize,
        .pDynamicStates    = dynamicStates,
        .pNext             = VK_NULL_HANDLE,
    };

    VkPipelineDepthStencilStateCreateInfo depthStencil = {
        .sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable       = VK_TRUE,
        .depthWriteEnable      = VK_TRUE,
        .depthCompareOp        = VK_COMPARE_OP_LESS,
        .depthBoundsTestEnable = VK_FALSE,
        .minDepthBounds        = 0.0f, // Optional
        .maxDepthBounds        = 1.0f, // Optional
        .stencilTestEnable     = VK_FALSE,
        .front                 = {}, // Optional
        .back                  = {}, // Optional
    };
    // PARTICLE VERTEX INPUT
    gdp->objectIADLen = 3;
    fillParticleVertexInputAttributeDescription(&gdp->objectIAD, gdp->objectIADLen);
    gdp->objectIBDLen = 1;
    fillParticleVertexBindingDescriptions(&gdp->objectIBD, VK_VERTEX_INPUT_RATE_VERTEX, sizeof(TrzVertex));

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {
        .sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount   = gdp->objectIBDLen,
        .pVertexBindingDescriptions      = gdp->objectIBD,
        .vertexAttributeDescriptionCount = gdp->objectIADLen,
        .pVertexAttributeDescriptions    = gdp->objectIAD,
        .pNext                           = VK_NULL_HANDLE,
    };

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE,
        .pNext                  = VK_NULL_HANDLE,
    };

    VkViewport viewport = {
        .x        = 0.0f,
        .y        = 0.0f,
        .width    = (float) dm->graphicPipeline->swapchain->currentExtent2D.width,
        .height   = (float) dm->graphicPipeline->swapchain->currentExtent2D.height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };

    VkRect2D scissor = {
        .offset = {0, 0},
        .extent = dm->graphicPipeline->swapchain->currentExtent2D,
    };

    VkPipelineViewportStateCreateInfo viewportState = {
        .sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .pViewports    = &viewport,
        .scissorCount  = 1,
        .pScissors     = &scissor,
        .pNext         = VK_NULL_HANDLE,
    };

    VkPipelineMultisampleStateCreateInfo multisampling = {
        .sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .sampleShadingEnable   = VK_FALSE,
        .rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT,
        .minSampleShading      = 1.0f,     // Optional
        .pSampleMask           = NULL,     // Optional
        .alphaToCoverageEnable = VK_FALSE, // Optional
        .alphaToOneEnable      = VK_FALSE, // Optional
        .pNext                 = VK_NULL_HANDLE,
    };
    VkPipelineRasterizationStateCreateInfo rasterizer = {
        .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable        = VK_FALSE,
        .polygonMode             = VK_POLYGON_MODE_FILL,
        .lineWidth               = 1.0f,
        .cullMode                = VK_CULL_MODE_NONE,
        .frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .depthBiasEnable         = VK_FALSE,
        .depthBiasConstantFactor = 0.0f, // Optional
        .depthBiasClamp          = 0.0f, // Optional
        .depthBiasSlopeFactor    = 0.0f, // Optional
        .pNext                   = VK_NULL_HANDLE,
    };

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {
        .colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
        .blendEnable         = VK_TRUE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,  // Optional
        .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO, // Optional
        .colorBlendOp        = VK_BLEND_OP_ADD,      // Optional
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,  // Optional
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO, // Optional
        .alphaBlendOp        = VK_BLEND_OP_ADD,      // Optional
    };

    VkPipelineColorBlendStateCreateInfo colorBlending = {
        .sType             = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable     = VK_FALSE,
        .logicOp           = VK_LOGIC_OP_COPY, // Optional
        .attachmentCount   = 1,
        .pAttachments      = &colorBlendAttachment,
        .blendConstants[0] = 0.3f, // Optional
        .blendConstants[1] = 0.4f, // Optional
        .blendConstants[2] = 0.5f, // Optional
        .blendConstants[3] = 0.0f, // Optional
        .pNext             = VK_NULL_HANDLE,
    };

    /*VkPushConstantRange pushConstantRange = {
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .offset = 0,
        .size = sizeof(PushConstantDisplayTimer)
    }*/
    ;
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
        .sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = gdp->maxFrames, // Optional
        .pSetLayouts    = (VkDescriptorSetLayout[]) {gdp->dsl, gdp->dsl, gdp->dsl},
        //.pushConstantRangeCount = 0,
        //.pPushConstantRanges = &pushConstantRange, // Optional
        .pNext = VK_NULL_HANDLE,
    };

    gdp->pipelineLayout = malloc(sizeof(*gdp->pipelineLayout));
    if (vkCreatePipelineLayout(*device, &pipelineLayoutInfo, NULL, gdp->pipelineLayout) != VK_SUCCESS)
    {

        exit(1);
    }

    // createRenderPass(dm->swapchain,*device,gdp);

    VkGraphicsPipelineCreateInfo pipelineInfo = {
        .sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount          = 2,
        .pStages             = &shaderStages[0],
        .pVertexInputState   = &vertexInputInfo,
        .pInputAssemblyState = &inputAssembly,
        .pViewportState      = &viewportState,
        .pRasterizationState = &rasterizer,
        .pMultisampleState   = &multisampling,
        .pDepthStencilState  = NULL, // Optional
        .pColorBlendState    = &colorBlending,
        .pDynamicState       = &dynamicState,
        .layout              = *gdp->pipelineLayout,
        .renderPass          = *gdp->renderPass,
        .subpass             = 2,
        .pDepthStencilState  = &depthStencil,
        .basePipelineHandle  = VK_NULL_HANDLE, // Optional
        .basePipelineIndex   = -1,             // Optional
        .pNext               = VK_NULL_HANDLE,
    };
    gdp->pipeline = malloc(sizeof(*gdp->pipeline));
    if (vkCreateGraphicsPipelines(*device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, gdp->pipeline) != VK_SUCCESS)
    {

        exit(1);
    }
}

void createLightVolumeGraphicPipeline(GraphicPipelineToSurface* gdp, DisplayManager* dm, VkDevice* device)
{
    char** lightVert = (char**) malloc(sizeof(char*));
    char** lightFrag = (char**) malloc(sizeof(char*));

    size_t lightVertSize = 0;
    size_t lightFragSize = 0;

    readSPVBinaryFile(VOLUMETRIC_LIGHT_VERT_SHADER, lightVert, &lightVertSize);
    readSPVBinaryFile(VOLUMETRIC_LIGHT_FRAG_SHADER, lightFrag, &lightFragSize);
    // TODOcrear struct para spv, guarde datos y un hilo los lea y escriba en background
    assert(lightVertSize > 0);
    assert(lightFragSize > 0);

    gdp->vertShader = createShaderModule(*device, lightVert, lightVertSize);
    gdp->fragShader = createShaderModule(*device, lightFrag, lightFragSize);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo = {
        .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage  = VK_SHADER_STAGE_VERTEX_BIT,
        .module = (*gdp->vertShader),
        .pName  = "main",
        .pNext  = VK_NULL_HANDLE,
    };

    VkPipelineShaderStageCreateInfo fragShaderStageInfo = {
        .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage  = VK_SHADER_STAGE_FRAGMENT_BIT,
        .module = (*gdp->fragShader),
        .pName  = "main",
        .pNext  = VK_NULL_HANDLE,
    };

    VkPipelineShaderStageCreateInfo shaderStages[2];
    shaderStages[0] = vertShaderStageInfo;
    shaderStages[1] = fragShaderStageInfo;

    VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

    uint32_t dynamicStateSize = 2;

    VkPipelineDynamicStateCreateInfo dynamicState = {
        .sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = dynamicStateSize,
        .pDynamicStates    = dynamicStates,
        .pNext             = VK_NULL_HANDLE,
    };

    VkPipelineDepthStencilStateCreateInfo depthStencil = {
        .sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable       = VK_TRUE,
        .depthWriteEnable      = VK_TRUE,
        .depthCompareOp        = VK_COMPARE_OP_ALWAYS,
        .depthBoundsTestEnable = VK_FALSE,
        .minDepthBounds        = 0.0f, // Optional
        .maxDepthBounds        = 1.0f, // Optional
        .stencilTestEnable     = VK_FALSE,
        .front                 = {}, // Optional
        .back                  = {}, // Optional
    };

    // PARTICLE VERTEX INPUT
    gdp->objectIADLen = 2;
    fillLightVolumeVertexInputAttributeDescription(&gdp->objectIAD, gdp->objectIADLen);
    gdp->objectIBDLen = 1;
    fillLightVolumeBindingDescriptions(&gdp->objectIBD, VK_VERTEX_INPUT_RATE_VERTEX, sizeof(GEOM_VERTEX));

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {
        .sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount   = gdp->objectIBDLen,
        .pVertexBindingDescriptions      = gdp->objectIBD,
        .vertexAttributeDescriptionCount = gdp->objectIADLen,
        .pVertexAttributeDescriptions    = gdp->objectIAD,
        .pNext                           = VK_NULL_HANDLE,
    };

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE,
        .pNext                  = VK_NULL_HANDLE,
    };

    VkViewport viewport = {
        .x        = 0.0f,
        .y        = 0.0f,
        .width    = (float) dm->graphicPipeline->swapchain->currentExtent2D.width,
        .height   = (float) dm->graphicPipeline->swapchain->currentExtent2D.height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };

    VkRect2D scissor = {
        .offset = {0, 0},
        .extent = dm->graphicPipeline->swapchain->currentExtent2D,
    };

    VkPipelineViewportStateCreateInfo viewportState = {
        .sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .pViewports    = &viewport,
        .scissorCount  = 1,
        .pScissors     = &scissor,
        .pNext         = VK_NULL_HANDLE,
    };

    VkPipelineMultisampleStateCreateInfo multisampling = {
        .sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .sampleShadingEnable   = VK_FALSE,
        .rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT,
        .minSampleShading      = 1.0f,     // Optional
        .pSampleMask           = NULL,     // Optional
        .alphaToCoverageEnable = VK_FALSE, // Optional
        .alphaToOneEnable      = VK_FALSE, // Optional
        .pNext                 = VK_NULL_HANDLE,
    };
    VkPipelineRasterizationStateCreateInfo rasterizer = {
        .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable        = VK_FALSE,
        .polygonMode             = VK_POLYGON_MODE_FILL,
        .lineWidth               = 1.0f,
        .cullMode                = VK_CULL_MODE_FRONT_BIT,
        .frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .depthBiasEnable         = VK_FALSE,
        .depthBiasConstantFactor = 0.0f, // Optional
        .depthBiasClamp          = 0.0f, // Optional
        .depthBiasSlopeFactor    = 0.0f, // Optional
        .pNext                   = VK_NULL_HANDLE,
    };

    VkPipelineColorBlendAttachmentState colorBlendAttachment[3] = {
        {

            .blendEnable         = VK_TRUE,
            .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
            .dstColorBlendFactor = VK_BLEND_FACTOR_ONE,
            .colorBlendOp        = VK_BLEND_OP_ADD,
            .colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
                              VK_COLOR_COMPONENT_A_BIT,
        },
    };

    uint32_t colorBlendAttachmentLen = 1;

    VkPipelineColorBlendStateCreateInfo colorBlending = {
        .sType             = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable     = VK_FALSE,
        .logicOp           = VK_LOGIC_OP_COPY, // Optional
        .attachmentCount   = colorBlendAttachmentLen,
        .pAttachments      = colorBlendAttachment,
        .blendConstants[0] = 0.3f, // Optional
        .blendConstants[1] = 0.4f, // Optional
        .blendConstants[2] = 0.5f, // Optional
        .blendConstants[3] = 0.0f, // Optional
        .pNext             = VK_NULL_HANDLE,
    };

    /*VkPushConstantRange pushConstantRange = {
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .offset = 0,
        .size = sizeof(PushConstantDisplayTimer)
    }*/
    ;
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
        .sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = gdp->maxFrames, // Optional
        .pSetLayouts    = (VkDescriptorSetLayout[]) {gdp->dsl, gdp->dsl, gdp->dsl},
        //.pushConstantRangeCount = 0,
        //.pPushConstantRanges = &pushConstantRange, // Optional
        .pNext = VK_NULL_HANDLE,
    };

    gdp->pipelineLayout = malloc(sizeof(*gdp->pipelineLayout));
    if (vkCreatePipelineLayout(*device, &pipelineLayoutInfo, NULL, gdp->pipelineLayout) != VK_SUCCESS)
    {

        exit(1);
    }

    // createRenderPass(dm->swapchain,*device,gdp);

    VkGraphicsPipelineCreateInfo pipelineInfo = {
        .sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount          = 2,
        .pStages             = &shaderStages[0],
        .pVertexInputState   = &vertexInputInfo,
        .pInputAssemblyState = &inputAssembly,
        .pViewportState      = &viewportState,
        .pRasterizationState = &rasterizer,
        .pMultisampleState   = &multisampling,
        .pDepthStencilState  = &depthStencil, // Optional
        .pColorBlendState    = &colorBlending,
        .pDynamicState       = &dynamicState,
        .layout              = *gdp->pipelineLayout,
        .renderPass          = *gdp->renderPass,
        .subpass             = 1,
        .basePipelineHandle  = VK_NULL_HANDLE, // Optional
        .basePipelineIndex   = -1,             // Optional
        .pNext               = VK_NULL_HANDLE,
    };
    gdp->pipeline = malloc(sizeof(*gdp->pipeline));
    if (vkCreateGraphicsPipelines(*device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, gdp->pipeline) != VK_SUCCESS)
    {

        exit(1);
    }
}

#define DEPTH_PREPASS_VERT_SHADER "shaders/bin/depthPrepass.vert.spv"
// No fragment shader needed - early depth writes only

void createDepthPrepassGraphicsPipeline(GraphicPipelineToSurface* depthGdp, DisplayManager* dm, VkDevice* device)
{
    // Load only vertex shader
    char** depthVert     = malloc(sizeof(char*));
    size_t depthVertSize = 0;
    readSPVBinaryFile(DEPTH_PREPASS_VERT_SHADER, depthVert, &depthVertSize);

    depthGdp->vertShader = createShaderModule(*device, depthVert, depthVertSize);
    depthGdp->fragShader = NULL; // No fragment shader for depth prepass

    // Single stage - vertex only
    VkPipelineShaderStageCreateInfo vertStage = {.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                                                 .stage  = VK_SHADER_STAGE_VERTEX_BIT,
                                                 .module = (*depthGdp->vertShader),
                                                 .pName  = "main",
                                                 .pNext  = VK_NULL_HANDLE};

    // Use your existing vertex input setup (models use GEOM_VERTEX)
    depthGdp->objectIADLen = 2; // Position + normal (same as models)
    fillDepthPassVertexInputAttributeDescription(&depthGdp->objectIAD, depthGdp->objectIADLen);
    depthGdp->objectIBDLen = 1;
    fillDepthPassVertexBindingDescriptions(&depthGdp->objectIBD, VK_VERTEX_INPUT_RATE_VERTEX, sizeof(GEOM_VERTEX));

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {
        .sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount   = depthGdp->objectIBDLen,
        .pVertexBindingDescriptions      = depthGdp->objectIBD,
        .vertexAttributeDescriptionCount = depthGdp->objectIADLen,
        .pVertexAttributeDescriptions    = depthGdp->objectIAD,
        .pNext                           = VK_NULL_HANDLE};

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE,
        .pNext                  = VK_NULL_HANDLE};

    // Dynamic viewport and scissor (matching your existing pattern)
    VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

    VkPipelineDynamicStateCreateInfo dynamicState = {.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
                                                     .dynamicStateCount = 2,
                                                     .pDynamicStates    = dynamicStates,
                                                     .pNext             = VK_NULL_HANDLE};

    VkPipelineViewportStateCreateInfo viewportState = {.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
                                                       .viewportCount = 1,
                                                       .pViewports    = NULL, // Dynamic
                                                       .scissorCount  = 1,
                                                       .pScissors     = NULL, // Dynamic
                                                       .pNext         = VK_NULL_HANDLE};

    // Early fragment test enablement via rasterization state
    VkPipelineRasterizationStateCreateInfo rasterizer = {
        .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable        = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE, // Must be false to write depth
        .polygonMode             = VK_POLYGON_MODE_FILL,
        .lineWidth               = 1.0f,
        .cullMode                = VK_CULL_MODE_BACK_BIT, // Cull back faces for efficiency
        .frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .depthBiasEnable         = VK_FALSE,
        .pNext                   = VK_NULL_HANDLE};

    // No multisampling for depth prepass (keep it simple and fast)
    VkPipelineMultisampleStateCreateInfo multisampling = {.sType =
                                                              VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
                                                          .sampleShadingEnable   = VK_FALSE,
                                                          .rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT,
                                                          .minSampleShading      = 1.0f,
                                                          .pSampleMask           = NULL,
                                                          .alphaToCoverageEnable = VK_FALSE,
                                                          .alphaToOneEnable      = VK_FALSE,
                                                          .pNext                 = VK_NULL_HANDLE};

    // Aggressive depth state for prepass
    VkPipelineDepthStencilStateCreateInfo depthStencil = {
        .sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable       = VK_TRUE,
        .depthWriteEnable      = VK_TRUE,
        .depthCompareOp        = VK_COMPARE_OP_LESS, // Early-Z optimization friendly
        .depthBoundsTestEnable = VK_FALSE,
        .minDepthBounds        = 0.0f,
        .maxDepthBounds        = 1.0f,
        .stencilTestEnable     = VK_FALSE,
        .front                 = {},
        .back                  = {}};

    // Critical: Disable color writes completely
    VkPipelineColorBlendStateCreateInfo colorBlending = {.sType =
                                                             VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
                                                         .logicOpEnable   = VK_FALSE,
                                                         .attachmentCount = 0, // No color attachments
                                                         .pAttachments    = NULL,
                                                         .pNext           = VK_NULL_HANDLE};

    // Create pipeline layout (reuse your existing descriptor set layout)
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
        .sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = depthGdp->maxFrames,
        .pSetLayouts =
            (VkDescriptorSetLayout[]) {depthGdp->dsl, depthGdp->dsl, depthGdp->dsl}, // Same pattern as your code
        .pushConstantRangeCount = 0,
        .pPushConstantRanges    = NULL,
        .pNext                  = VK_NULL_HANDLE};

    depthGdp->pipelineLayout = malloc(sizeof(*depthGdp->pipelineLayout));
    if (vkCreatePipelineLayout(*device, &pipelineLayoutInfo, NULL, depthGdp->pipelineLayout) != VK_SUCCESS)
    {
        exit(1);
    }

    // Create the graphics pipeline
    VkGraphicsPipelineCreateInfo pipelineInfo = {.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
                                                 .stageCount          = 1, // Only vertex shader
                                                 .pStages             = &vertStage,
                                                 .pVertexInputState   = &vertexInputInfo,
                                                 .pInputAssemblyState = &inputAssembly,
                                                 .pViewportState      = &viewportState,
                                                 .pRasterizationState = &rasterizer,
                                                 .pMultisampleState   = &multisampling,
                                                 .pDepthStencilState  = &depthStencil,
                                                 .pColorBlendState    = &colorBlending,
                                                 .pDynamicState       = &dynamicState,
                                                 .layout              = *depthGdp->pipelineLayout,
                                                 .renderPass =
                                                     *depthGdp->renderPass, // You'll need the depth-only render pass
                                                 .subpass            = 0,
                                                 .basePipelineHandle = VK_NULL_HANDLE,
                                                 .basePipelineIndex  = -1,
                                                 .pNext              = VK_NULL_HANDLE};

    depthGdp->pipeline = malloc(sizeof(*depthGdp->pipeline));
    if (vkCreateGraphicsPipelines(*device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, depthGdp->pipeline) != VK_SUCCESS)
    {
        exit(1);
    }

    // Clean up
    free(depthVert);
}
