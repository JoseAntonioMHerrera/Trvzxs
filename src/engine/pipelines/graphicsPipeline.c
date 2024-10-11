#include "graphicsPipeline.h"

#define TRIANGLE_VERT_SHADER "shaders/bin/triangle.vert.spv\0"
#define TRIANGLE_FRAG_SHADER "shaders/bin/triangle.frag.spv\0"

 void createSyncObjects(GraphicPipelineToSurface *gd, VkDevice *device){
    VkSemaphoreCreateInfo semaphoreInfo = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    };

    VkFenceCreateInfo fenceInfo = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT,
    };
    assert(gd->commandLen >= 2);
    gd->imgViewSemaphore = malloc(sizeof(VkSemaphore) * gd->commandLen);
    gd->renderEndFlag = malloc(sizeof(VkSemaphore) * gd->commandLen);
    gd->inFlightFence = malloc(sizeof(VkFence) * gd->commandLen);

    assert(gd->inFlightFence);
    assert(gd->renderEndFlag);
    assert(gd->imgViewSemaphore);

    for(uint32_t i = 0; i < gd->commandLen; i++) {
        if (vkCreateSemaphore(*device, &semaphoreInfo, NULL, &gd->imgViewSemaphore[i]) != VK_SUCCESS ||
        vkCreateSemaphore(*device, &semaphoreInfo, NULL, &gd->renderEndFlag[i]) != VK_SUCCESS ||
        vkCreateFence(*device, &fenceInfo, NULL, &gd->inFlightFence[i]) != VK_SUCCESS) {

        }
    }

}

 void startRenderPass(GraphicPipelineToSurface *gdp, DisplayManager *dm, VkFramebuffer swapchainFramebuffer){
    VkRenderPassBeginInfo renderPassInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = *gdp->renderPass,
        .framebuffer = swapchainFramebuffer,
        .renderArea.offset = {0, 0},
        .renderArea.extent = dm->graphicPipeline->swapchain->currentExtent2D,

    };

    VkClearValue clearValues[2];
    clearValues[0].color = (VkClearColorValue){0.0f, 0.0f, 0.0f, 1.0f};
    clearValues[1].depthStencil = (VkClearDepthStencilValue){1.0f, 0};
    renderPassInfo.clearValueCount = 2;
    renderPassInfo.pClearValues = clearValues;

    vkCmdBeginRenderPass(gdp->commandBuffer[dm->currentFrame], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(gdp->commandBuffer[dm->currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, *gdp->pipeline);


    VkViewport viewport = {
        viewport.x = 0.0f,
        viewport.y = 0.0f,
        viewport.width = (float)(dm->graphicPipeline->swapchain->currentExtent2D.width),
        viewport.height = (float)(dm->graphicPipeline->swapchain->currentExtent2D.height),
        viewport.minDepth = 0.0f,
        viewport.maxDepth = 1.0f,
    };

    vkCmdSetViewport(gdp->commandBuffer[dm->currentFrame], 0, 1, &viewport);

    VkRect2D scissor = {
        .offset = {0, 0},
        .extent = dm->graphicPipeline->swapchain->currentExtent2D,
    };

    vkCmdSetScissor(gdp->commandBuffer[dm->currentFrame], 0, 1, &scissor);
    //vkCmdDraw(commandBuffers[currentFrame], objects.dataLen, 1, 0, 0);
    vkCmdBindDescriptorSets(gdp->commandBuffer[dm->currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, *gdp->pipelineLayout,
                            0, 1, &gdp->descriptorSet[dm->currentFrame], 0, VK_NULL_HANDLE);
    // vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);


    vkCmdDrawIndexed(gdp->commandBuffer[dm->currentFrame], gdp->geometry->totalIndicesLen, 1, 0, 0, 0); //TODO numero de indices a dibujar
    vkCmdEndRenderPass(gdp->commandBuffer[dm->currentFrame]);

}

 void recordCommandBuffer(GraphicPipelineToSurface *gdp, DisplayManager* dm, uint32_t imageIndex) {
    VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = 0, // Optional
        .pInheritanceInfo = NULL, // Optional
    };




    if (vkBeginCommandBuffer(gdp->commandBuffer[imageIndex], &beginInfo) != VK_SUCCESS) {

    }

    VkDeviceSize offsets[] = {0};
    const VkBuffer localBuffer[] = {*gdp->deviceBuffer->buffer};
    //updatePushConstants(gdp->commandBuffer[imageIndex], gdp->pipelineLayout);
    vkCmdBindVertexBuffers(gdp->commandBuffer[imageIndex], 0, 1, localBuffer, offsets);
    vkCmdBindIndexBuffer(gdp->commandBuffer[imageIndex], *gdp->deviceBuffer->indexBuffer, 0, VK_INDEX_TYPE_UINT32);


    startRenderPass(gdp, dm, dm->graphicPipeline->swapchain->p_framebuffers[imageIndex]);

}

 VkShaderModule* createShaderModule(VkDevice device, char** code, size_t codeSize) {
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

 void fillObjectBindingDescriptions(VkVertexInputBindingDescription **objectIBD){

    *objectIBD = malloc(sizeof(VkVertexInputBindingDescription));
    (*objectIBD)->binding = 0;
    (*objectIBD)->stride = sizeof(GEOM_VERTEX);
    (*objectIBD)->inputRate = VK_VERTEX_INPUT_RATE_VERTEX;


}

 void fillObjectInputAttributeDescription(VkVertexInputAttributeDescription **objectsIAD) { // MOVE  object input attribute description to each primitive
    *objectsIAD = (VkVertexInputAttributeDescription*) malloc(sizeof(VkVertexInputAttributeDescription)*3);
    // GEOM_OBJECT coordinates

    (*objectsIAD)[0].binding = 0;
    (*objectsIAD)[0].location = 0;
    (*objectsIAD)[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    (*objectsIAD)[0].offset = offsetof(GEOM_VERTEX, point);


    // GEOM_OBJECT colors
    (*objectsIAD)[1].binding = 0;
    (*objectsIAD)[1].location = 1;
    (*objectsIAD)[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    (*objectsIAD)[1].offset = offsetof(GEOM_VERTEX, color);


    // GEOM_OBJECT colors
    (*objectsIAD)[2].binding = 0;
    (*objectsIAD)[2].location = 2;
    (*objectsIAD)[2].format = VK_FORMAT_R32G32B32_SFLOAT;
    (*objectsIAD)[2].offset = offsetof(GEOM_VERTEX, textCoords);
}

 void createRenderPass(PhysicalDevice *pd, GraphicSwapchain *dsm, VkDevice device, GraphicPipelineToSurface *gdp) {
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
        .samples = dsm->msaaDepthRes->numSamples,
    };

    VkAttachmentDescription colorAttachment = {
        .format = (dsm->currentSurfaceFormat)->format,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .samples = dsm->msaaColorRes->numSamples,
    };
        VkAttachmentReference colorAttachmentRef = {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };



    VkAttachmentDescription colorAttachmentResolve = {
        .format = (dsm->currentSurfaceFormat)->format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    };

    VkAttachmentReference colorAttachmentResolveRef = {
        .attachment = 2,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };


    VkSubpassDescription subpass = {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachmentRef,
        .pDepthStencilAttachment = &depthAttachmentRef,
        .pResolveAttachments = &colorAttachmentResolveRef,
    };


    // TODO MANAGE SUBPASS TREE

    dsm->subpassDependency = malloc(sizeof(VkSubpassDependency)); //TODO this subpassDependency doesnt need to be  a pointer
    dsm->subpassDependency->srcSubpass = VK_SUBPASS_EXTERNAL;
    dsm->subpassDependency->dstSubpass = 0;
    dsm->subpassDependency->srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;;
    dsm->subpassDependency->srcAccessMask = 0;
    dsm->subpassDependency->dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dsm->subpassDependency->dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;;
    dsm->subpassDependency->dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
    VkAttachmentDescription attatchments[3] = {colorAttachment, depthAttachment, colorAttachmentResolve};
    VkRenderPassCreateInfo renderPassInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 3,
        .pAttachments = attatchments,
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = dsm->subpassDependency,
    };

    gdp->renderPass = malloc(sizeof(*gdp->renderPass));
    if (vkCreateRenderPass(device, &renderPassInfo, NULL, gdp->renderPass) != VK_SUCCESS) {

        exit(0);
    }


}

 void createGraphicsPipeline(GraphicPipelineToSurface *gdp, DisplayManager *dm, VkDevice* device) {
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
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
        .module = (*gdp->vertShader),
        .pName = "main",
        .pNext = VK_NULL_HANDLE,
    };

    VkPipelineShaderStageCreateInfo fragShaderStageInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        .module = (*gdp->fragShader),
        .pName = "main",
        .pNext = VK_NULL_HANDLE,
    };


    VkPipelineShaderStageCreateInfo shaderStages[2];
    shaderStages[0] = fragShaderStageInfo;
    shaderStages[1] = vertShaderStageInfo;

    VkDynamicState dynamicStates[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };


    uint32_t dynamicStateSize = 2;

    VkPipelineDynamicStateCreateInfo dynamicState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = dynamicStateSize,
        .pDynamicStates = dynamicStates,
        .pNext = VK_NULL_HANDLE,
    };

    VkPipelineDepthStencilStateCreateInfo depthStencil = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = VK_TRUE,
        .depthWriteEnable = VK_TRUE,
        .depthCompareOp = VK_COMPARE_OP_LESS,
        .depthBoundsTestEnable = VK_FALSE,
        .minDepthBounds = 0.0f, // Optional
        .maxDepthBounds = 1.0f, // Optional
        .stencilTestEnable = VK_FALSE,
        .front = {}, // Optional
        .back = {}, // Optional
    };




    // VERTEX INPUT
    gdp->objectIADLen = 3;
    fillObjectInputAttributeDescription(&gdp->objectIAD);
    gdp->objectIBDLen = 1;
    fillObjectBindingDescriptions(&gdp->objectIBD);


    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = gdp->objectIBDLen,
        .pVertexBindingDescriptions = gdp->objectIBD,
        .vertexAttributeDescriptionCount = gdp->objectIADLen,
        .pVertexAttributeDescriptions = gdp->objectIAD,
        .pNext = VK_NULL_HANDLE,
    };


    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE,
        .pNext = VK_NULL_HANDLE,
    };


    VkViewport viewport = {
        .x = 0.0f,
        .y = 0.0f,
        .width = (float) dm->graphicPipeline->swapchain->currentExtent2D.width,
        .height = (float) dm->graphicPipeline->swapchain->currentExtent2D.height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };

    VkRect2D scissor = {
        .offset = {0, 0},
        .extent = dm->graphicPipeline->swapchain->currentExtent2D,
    };

    VkPipelineViewportStateCreateInfo viewportState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .pViewports = &viewport,
        .scissorCount = 1,
        .pScissors = &scissor,
        .pNext = VK_NULL_HANDLE,
    };

    VkPipelineRasterizationStateCreateInfo rasterizer = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .lineWidth = 1.0f,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .depthBiasConstantFactor = 0.0f, // Optional
        .depthBiasClamp = 0.0f, // Optional
        .depthBiasSlopeFactor = 0.0f, // Optional
        .pNext = VK_NULL_HANDLE,
    };


    VkPipelineMultisampleStateCreateInfo multisampling = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .sampleShadingEnable = VK_FALSE,
        .rasterizationSamples = dm->graphicPipeline->swapchain->msaaColorRes->numSamples,
        .minSampleShading = 1.0f, // Optional
        .pSampleMask = NULL, // Optional
        .alphaToCoverageEnable = VK_FALSE, // Optional
        .alphaToOneEnable = VK_FALSE, // Optional
        .pNext = VK_NULL_HANDLE,
    };

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
        .blendEnable = VK_TRUE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_ONE, // Optional
        .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO, // Optional
        .colorBlendOp = VK_BLEND_OP_ADD, // Optional
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE, // Optional
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO, // Optional
        .alphaBlendOp = VK_BLEND_OP_ADD, // Optional
    };

    VkPipelineColorBlendStateCreateInfo colorBlending = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY, // Optional
        .attachmentCount = 1,
        .pAttachments = &colorBlendAttachment,
        .blendConstants[0] = 0.3f, // Optional
        .blendConstants[1] = 0.4f, // Optional
        .blendConstants[2] = 0.5f, // Optional
        .blendConstants[3] = 0.0f, // Optional
        .pNext = VK_NULL_HANDLE,
    };

    /*VkPushConstantRange pushConstantRange = {
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .offset = 0,
        .size = sizeof(PushConstantDisplayTimer)
    }*/;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = gdp->maxFrames, // Optional
        .pSetLayouts = (VkDescriptorSetLayout[]){gdp->dsl, gdp->dsl, gdp->dsl},
        //.pushConstantRangeCount = 0,
        //.pPushConstantRanges = &pushConstantRange, // Optional
        .pNext = VK_NULL_HANDLE,
    };


    gdp->pipelineLayout = malloc(sizeof(*gdp->pipelineLayout));
    if (vkCreatePipelineLayout(*device, &pipelineLayoutInfo, NULL, gdp->pipelineLayout) != VK_SUCCESS) {

        exit(1);
    }

    //createRenderPass(dm->swapchain,*device,gdp);


    VkGraphicsPipelineCreateInfo pipelineInfo = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = 2,
        .pStages = &shaderStages[0],
        .pVertexInputState = &vertexInputInfo,
        .pInputAssemblyState = &inputAssembly,
        .pViewportState = &viewportState,
        .pRasterizationState = &rasterizer,
        .pMultisampleState = &multisampling,
        .pDepthStencilState = NULL, // Optional
        .pColorBlendState = &colorBlending,
        .pDynamicState = &dynamicState,
        .layout = *gdp->pipelineLayout,
        .renderPass = *gdp->renderPass,
        .subpass = 0,
        .pDepthStencilState = &depthStencil,
        .basePipelineHandle = VK_NULL_HANDLE, // Optional
        .basePipelineIndex = -1, // Optional
        .pNext = VK_NULL_HANDLE,
    };
    gdp->pipeline = malloc(sizeof(*gdp->pipeline));
    if (vkCreateGraphicsPipelines(*device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, gdp->pipeline) != VK_SUCCESS) {

        exit(1);
    }


}
