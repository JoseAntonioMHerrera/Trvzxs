#pragma once
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>
#include "transformations.h"
#include "file_functions.h"
#include "buffer.h"
#include "transformations.h"
#include "vertex.h"
#include "numberTypes.h"
#include "geometryPipelineTypes.h"
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>




TRVZXS_inline void createSyncObjects(GeometryDisplayPipelineManager *gd, VkDevice *device){
    VkSemaphoreCreateInfo semaphoreInfo = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    };

    VkFenceCreateInfo fenceInfo = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT,
    };
    assert(gd->commandBuffer >= 2);
    gd->imgViewSemaphore = malloc(sizeof(VkSemaphore) * gd->commandLen);
    gd->renderEndFlag = malloc(sizeof(VkSemaphore) * gd->commandLen);
    gd->inFlightFence = malloc(sizeof(VkFence) * gd->commandLen);

    assert(gd->inFlightFence);
    assert(gd->renderEndFlag);
    assert(gd->imgViewSemaphore);

    for(uint32_t i = 0; i < gd->commandLen; i++) {
        if (vkCreateSemaphore(device, &semaphoreInfo, NULL, &gd->imgViewSemaphore[i]) != VK_SUCCESS ||
        vkCreateSemaphore(device, &semaphoreInfo, NULL, &gd->renderEndFlag[i]) != VK_SUCCESS ||
        vkCreateFence(device, &fenceInfo, NULL, &gd->inFlightFence[i]) != VK_SUCCESS) {
            printf("failed to create semaphores!");
        }
    }

}

TRVZXS_inline void startRenderPass(GeometryDisplayPipelineManager *gdp, DisplayManager *dm, VkFramebuffer swapchainFramebuffer){
    VkRenderPassBeginInfo renderPassInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = gdp->renderPass,
        .framebuffer = swapchainFramebuffer,
        .renderArea.offset = {0, 0},
        .renderArea.extent = dm->swapChainM->currentExtent2D,

    };
    VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(gdp->commandBuffer[dm->currentFrame], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(gdp->commandBuffer[dm->currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, gdp->pipeline);

    VkViewport viewport = {
        viewport.x = 0.0f,
        viewport.y = 0.0f,
        viewport.width = (float)(dm->swapChainM->currentExtent2D.width),
        viewport.height = (float)(dm->swapChainM->currentExtent2D.height),
        viewport.minDepth = 0.0f,
        viewport.maxDepth = 1.0f,
    };

    vkCmdSetViewport(gdp->commandBuffer[dm->currentFrame], 0, 1, &viewport);

    VkRect2D scissor = {
        .offset = {0, 0},
        .extent = dm->swapChainM->currentExtent2D,
    };

    vkCmdSetScissor(gdp->commandBuffer[dm->currentFrame], 0, 1, &scissor);
    //vkCmdDraw(commandBuffers[currentFrame], objects.dataLen, 1, 0, 0);
    vkCmdDrawIndexed(gdp->commandBuffer[dm->currentFrame], gdp->geometry->indicesLen, 1, 0, 0, 0);
    vkCmdEndRenderPass(gdp->commandBuffer[dm->currentFrame]);
}

TRVZXS_inline void recordCommandBuffer(GeometryDisplayPipelineManager *gdp, DisplayManager* dm, uint32_t imageIndex) {
    VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = 0, // Optional
        .pInheritanceInfo = NULL, // Optional
    };

    if (vkBeginCommandBuffer(gdp->commandBuffer, &beginInfo) != VK_SUCCESS) {
        printf("Error al iniciar el commandbuffer");
    }

    VkBuffer vertexBuffers[] = {gdp->geometry};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(gdp->commandBuffer[imageIndex], 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(gdp->commandBuffer[imageIndex], gdp->deviceBuffer->indexBuffer, 0, VK_INDEX_TYPE_UINT32);


    startRenderPass(gdp, dm, dm->swapChainM->p_framebuffers[imageIndex]);

}

TRVZXS_inline VkShaderModule createShaderModule(VkDevice device, char** code, size_t codeSize) {
    VkShaderModuleCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = codeSize,
        .pCode = (uint32_t*) *code,
    };
    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, VK_NULL_HANDLE, &shaderModule) != VK_SUCCESS) {
        printf("failed to create shader module!");
        exit(0);
    }
    return shaderModule;
}

TRVZXS_inline void fillObjectBindingDescriptions(VkVertexInputBindingDescription *objectIBD){

    objectIBD->binding = 0;
    objectIBD->stride = sizeof(GEOM_VERTEX);
    objectIBD->inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
}

TRVZXS_inline void fillObjectInputAttributeDescription(VkVertexInputAttributeDescription **objectsIAD) {

    *objectsIAD = (VkVertexInputAttributeDescription*) malloc(sizeof(VkVertexInputAttributeDescription)*2);
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
}

TRVZXS_inline void createRenderPass(DisplaySwapchainManager *dsm, VkDevice device, GeometryDisplayPipelineManager *gdp) {
    VkAttachmentDescription colorAttachment = {
        .format = (dsm->currentSurfaceFormat)->format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    };


    VkAttachmentReference colorAttachmentRef = {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };

    VkSubpassDescription subpass = {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachmentRef,
    };
    dsm->subpassDependency->srcSubpass = VK_SUBPASS_EXTERNAL;
    dsm->subpassDependency->dstSubpass = 0;
    dsm->subpassDependency->srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dsm->subpassDependency->srcAccessMask = 0;
    dsm->subpassDependency->dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dsm->subpassDependency->dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dsm->subpassDependency->dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
    VkRenderPassCreateInfo renderPassInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &colorAttachment,
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = &dsm->subpassDependency,
    };

    if (vkCreateRenderPass(device, &renderPassInfo, NULL, &gdp->renderPass) != VK_SUCCESS) {
        printf("Error creating the render pass!");
        exit(0);
    }

}

TRVZXS_inline void createGraphicsPipeline(GeometryDisplayPipelineManager *gdp, DisplayManager *dm, VkDevice* device) {
    char** triangleVert = (char**) malloc(sizeof(char*));
    char** triangleFrag = (char**) malloc(sizeof(char*));
    size_t triangleVertSize = 0;
    size_t triangleFragSize = 0;

    assert(readSPVBinaryFile("shaders/bin/triangleVert.spv\0", triangleVert, &triangleVertSize) == 0);
    assert(readSPVBinaryFile("shaders/bin/triangleFrag.spv\0", triangleFrag, &triangleFragSize) == 0);
    // TODOcrear struct para spv, guarde datos y un hilo los lea y escriba en background

    gdp->vertShader = createShaderModule(device, triangleVert, triangleVertSize);
    gdp->vertShader = createShaderModule(device, triangleFrag, triangleFragSize);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
        .module = gdp->vertShader,
        .pName = "main",
    };

    VkPipelineShaderStageCreateInfo fragShaderStageInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        .module = gdp->fragShader,
        .pName = "main",
    };

    VkPipelineShaderStageCreateInfo shaderStages[2];
    shaderStages[0] = fragShaderStageInfo;
    shaderStages[1] = vertShaderStageInfo;

    VkDynamicState dynamicStates[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    uint32_t dynamicStateSize = 2;

    VkPipelineDynamicStateCreateInfo dynamicState;
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = dynamicStateSize;
    dynamicState.pDynamicStates = dynamicStates;


    // VERTEX INPUT

    fillObjectInputAttributeDescription(gdp->objectIAD);
    fillObjectBindingDescriptions(gdp->objectIBD);

    VkPipelineVertexInputStateCreateInfo vertexInputInfo;
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = gdp->objectIBDLen;
    vertexInputInfo.pVertexBindingDescriptions = gdp->objectIBD;
    vertexInputInfo.vertexAttributeDescriptionCount = gdp->objectIADLen;
    vertexInputInfo.pVertexAttributeDescriptions = gdp->objectIAD;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly;
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport = {
        .x = 0.0f,
        .y = 0.0f,
        .width = (float) dm->swapChainM->currentExtent2D.width,
        .height = (float) dm->swapChainM->currentExtent2D.height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };

    VkRect2D scissor = {
        .offset = {0, 0},
        .extent = dm->swapChainM->currentExtent2D,
    };

    VkPipelineViewportStateCreateInfo viewportState;
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer;
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f; // Optional
    rasterizer.depthBiasClamp = 0.0f; // Optional
    rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

    VkPipelineMultisampleStateCreateInfo multisampling;
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading = 1.0f; // Optional
    multisampling.pSampleMask = NULL; // Optional
    multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
    multisampling.alphaToOneEnable = VK_FALSE; // Optional

    VkPipelineColorBlendAttachmentState colorBlendAttachment;
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

    VkPipelineColorBlendStateCreateInfo colorBlending;
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.3f; // Optional
    colorBlending.blendConstants[1] = 0.4f; // Optional
    colorBlending.blendConstants[2] = 0.5f; // Optional
    colorBlending.blendConstants[3] = 0.0f; // Optional

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 0, // Optional
        .pSetLayouts = NULL, // Optional
        .pushConstantRangeCount = 0, // Optional
        .pPushConstantRanges = NULL, // Optional
    };


    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, NULL, gdp->pipelineLayout) != VK_SUCCESS) {
        printf("failed to create pipeline layout!");
        exit(1);
    }

    createRenderPass(dm->swapChainM,device,gdp);

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
        .layout = gdp->pipelineLayout,
        .renderPass = gdp->renderPass,
        .subpass = 0,
        .basePipelineHandle = VK_NULL_HANDLE, // Optional
        .basePipelineIndex = -1, // Optional
    };

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, gdp->pipeline) != VK_SUCCESS) {
        printf("Error al crear el pipeline grafico");
        exit(1);
    }

}
