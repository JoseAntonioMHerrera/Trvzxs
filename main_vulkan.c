#include <SDL_stdinc.h>
#include <SDL_vulkan.h>
#include <stdint.h>
#include <time.h>
#include <vulkan/vulkan.h>
#include <assert.h>
#include "src/utils/include/string_helper_macros.h"
#include "src/utils/include/comparation_helper_macros.h"
#include <unistd.h>
#include <vulkan/vulkan_core.h>
#include "src/utils/fileFunctions/include/file_functions.h"
#include "src/include/geometry/vertex.h"
#include <vulkan/vulkan_core.h>
#include "src/utils/include/strictDebug.h"
#include "main_vulkan.h"
#include "include/cglm/cglm.h"

#include "src/engine/device/deviceManagment.c"
#include "src/engine/include/vulkanHut.h"

GEOM_OBJECT objects;
void createDescriptorSetLayout(){
    VkDescriptorSetLayoutBinding uboLayoutBinding ={
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1,
        .pImmutableSamplers = NULL,
    };
    VkDescriptorSetLayoutCreateInfo layoutInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = 1,
        .pBindings = &uboLayoutBinding,
     };


    if (vkCreateDescriptorSetLayout(device, &layoutInfo, NULL, &descriptorSetLayout) != VK_SUCCESS) {
        printf("failed to create descriptor set layout!\n");
        exit(1);
    }

}


void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
    VkCommandBufferAllocateInfo allocInfo;
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = transferCommandPool;
    allocInfo.commandBufferCount = 1;
    allocInfo.pNext = NULL;

    VkCommandBuffer transferCommandBuffer;
    vkAllocateCommandBuffers(device, &allocInfo, &transferCommandBuffer);
    VkCommandBufferBeginInfo beginInfo;
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT; // Command only executed once

    vkBeginCommandBuffer(transferCommandBuffer, &beginInfo);

    VkBufferCopy copyRegion;
    copyRegion.srcOffset = 0; // Optional
    copyRegion.dstOffset = 0; // Optional
    copyRegion.size = size;

    vkCmdCopyBuffer(transferCommandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
    vkEndCommandBuffer(transferCommandBuffer);
    VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &transferCommandBuffer,
        .pNext = NULL,
    };


    vkQueueSubmit(transferQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(transferQueue);

    vkFreeCommandBuffers(device, transferCommandPool, 1, &transferCommandBuffer);
}

void createVkBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkSharingMode sharingMode, VkMemoryPropertyFlags properties,
                    VkBuffer *buffer, VkDeviceMemory *bufferMemory) {

    VkBufferCreateInfo bufferInfo;
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = sharingMode;
    bufferInfo.flags = 0;
    bufferInfo.pNext = NULL;

    if (vkCreateBuffer(device, &bufferInfo, NULL, buffer) != VK_SUCCESS) {
        printf("Failed to create VkBuffer instance!");
        exit(1);
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, *buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo;
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);
    allocInfo.pNext = NULL;

    if (vkAllocateMemory(device, &allocInfo, NULL, bufferMemory) != VK_SUCCESS) {
        printf("Failed to allocate memory for VkBuffer");
    }

    vkBindBufferMemory(device, *buffer, *bufferMemory, 0);
}
void createIndiceBuffer(GEOM_OBJECT *objects) {
    uint32_t bufferSize = sizeof(uint32_t) * objects->indicesLen;

    VkBuffer stagingIndicesBuffer;
    VkDeviceMemory stagingIndicesBufferMemory;
    createVkBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_SHARING_MODE_EXCLUSIVE,
    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingIndicesBuffer,
    &stagingIndicesBufferMemory);

    void* data;
    vkMapMemory(device, stagingIndicesBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, objects->indices, (size_t) bufferSize);
    vkUnmapMemory(device, stagingIndicesBufferMemory);

    createVkBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE,
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &indexBuffer, &indexBufferMemory);

    copyBuffer(stagingIndicesBuffer, indexBuffer, bufferSize);

    vkDestroyBuffer(device, stagingIndicesBuffer, NULL);
    vkFreeMemory(device, stagingIndicesBufferMemory, NULL);

}
void createGeometryBuffer(GEOM_OBJECT *objects){
    uint32_t geometryBufferSize = sizeof(*(objects->data))*objects->dataLen;

    createVkBuffer(geometryBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_SHARING_MODE_EXCLUSIVE,
    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer, &stagingBufferMemory);

    void* memoryMappedData; //Dst memory to copy from buffer data to gpu memory
    vkMapMemory(device, stagingBufferMemory, 0, geometryBufferSize, 0, &memoryMappedData);
    memcpy(memoryMappedData, objects->data, (size_t) geometryBufferSize);
    vkUnmapMemory(device, stagingBufferMemory);
    printf("HOLEHOLAHOALSDJAPOSD\n");
    createVkBuffer(geometryBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE,
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &geometryBuffer, &geometryBufferMemory);

    copyBuffer(stagingBuffer, geometryBuffer, geometryBufferSize);

    vkDestroyBuffer(device, stagingBuffer, NULL);
    vkFreeMemory(device, stagingBufferMemory, NULL);


}



void fillObjectInputAttributeDescription() {

    // GEOM_OBJECT coordinates
    objectsIAD[0].binding = 0;
    objectsIAD[0].location = 0;
    objectsIAD[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    objectsIAD[0].offset = offsetof(GEOM_VERTEX, point);


    // GEOM_OBJECT colors
    objectsIAD[1].binding = 0;
    objectsIAD[1].location = 1;
    objectsIAD[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    objectsIAD[1].offset = offsetof(GEOM_VERTEX, color);
}


void fillObjectBindingDescriptions(){

    objectIBD.binding = 0;
    objectIBD.stride = sizeof(GEOM_VERTEX);
    objectIBD.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
}

void cleanupSyncObjects(){

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(device, renderFinishedSemaphore[i], NULL);
        vkDestroySemaphore(device, imageAvailableSemaphore[i], NULL);
        vkDestroyFence(device, inFlightFence[i], NULL);
    }

}

void destroyDebugCallbackForVulkan(VkInstance* instance, VkDebugUtilsMessengerEXT* callback_messenger){
    PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT = VK_NULL_HANDLE;
    vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(*instance, "vkDestroyDebugUtilsMessengerEXT\0");
    assert(vkDestroyDebugUtilsMessengerEXT != NULL);

    vkDestroyDebugUtilsMessengerEXT(*instance, *callback_messenger, NULL);

    printf("Vulkan log callback destroyed.\n");
}


void createCommandBuffer(){
    commandBuffers = malloc(sizeof(VkCommandBuffer)* MAX_FRAMES_IN_FLIGHT);
    commandBuffersCount = MAX_FRAMES_IN_FLIGHT;
    VkCommandBufferAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = graphicsCommandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = commandBuffersCount,
    };

    if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers) != VK_SUCCESS) {
        printf("Error al crear el command buffer");
        exit(0);
    }
}

void startRenderPass(uint32_t imageIndex){
    VkRenderPassBeginInfo renderPassInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = renderPass,
        .framebuffer = swapChainFramebuffers[imageIndex],
        .renderArea.offset = {0, 0},
        .renderArea.extent = swapChainExtent,

    };
    VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(commandBuffers[currentFrame], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(commandBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

    VkViewport viewport = {
        viewport.x = 0.0f,
        viewport.y = 0.0f,
        viewport.width = (float)(swapChainExtent.width),
        viewport.height = (float)(swapChainExtent.height),
        viewport.minDepth = 0.0f,
        viewport.maxDepth = 1.0f,
    };

    vkCmdSetViewport(commandBuffers[currentFrame], 0, 1, &viewport);

    VkRect2D scissor = {
        .offset = {0, 0},
        .extent = swapChainExtent,
    };

    vkCmdSetScissor(commandBuffers[currentFrame], 0, 1, &scissor);
    //vkCmdDraw(commandBuffers[currentFrame], objects.dataLen, 1, 0, 0);
    vkCmdDrawIndexed(commandBuffers[currentFrame], objects.indicesLen, 1, 0, 0, 0);
    vkCmdEndRenderPass(commandBuffers[currentFrame]);
}
void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
    VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = 0, // Optional
        .pInheritanceInfo = NULL, // Optional
    };


    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        printf("Error al iniciar el commandbuffer");
    }

    VkBuffer vertexBuffers[] = {geometryBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffers[currentFrame], 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffers[currentFrame], indexBuffer, 0, VK_INDEX_TYPE_UINT32);


    startRenderPass(imageIndex);

}

void createSyncObjects(){
    VkSemaphoreCreateInfo semaphoreInfo = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    };

    VkFenceCreateInfo fenceInfo = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT,
    };
    assert(commandBuffersCount >= 2);
    imageAvailableSemaphore = malloc(sizeof(VkSemaphore) * commandBuffersCount);
    renderFinishedSemaphore = malloc(sizeof(VkSemaphore) * commandBuffersCount);
    inFlightFence = malloc(sizeof(VkFence) * commandBuffersCount);
    assert(inFlightFence);
    assert(renderFinishedSemaphore);
    assert(imageAvailableSemaphore);

    for(uint32_t i = 0; i < commandBuffersCount; i++) {
        if (vkCreateSemaphore(device, &semaphoreInfo, NULL, &imageAvailableSemaphore[i]) != VK_SUCCESS ||
        vkCreateSemaphore(device, &semaphoreInfo, NULL, &renderFinishedSemaphore[i]) != VK_SUCCESS ||
        vkCreateFence(device, &fenceInfo, NULL, &inFlightFence[i]) != VK_SUCCESS) {
            printf("failed to create semaphores!");
        }
    }

}



void createImageViews(){
    swapChainImageViewsCount = swapChainImageCount;
    swapChainImageViews = malloc(sizeof(VkImageView) * swapChainImageViewsCount);
    for (size_t i = 0; i < swapChainImageViewsCount; i++) {
        VkImageViewCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = swapChainImages[i],
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = selectedSurfaceFormat.format,
        .components.r = VK_COMPONENT_SWIZZLE_IDENTITY,
        .components.g = VK_COMPONENT_SWIZZLE_IDENTITY,
        .components.b = VK_COMPONENT_SWIZZLE_IDENTITY,
        .components.a = VK_COMPONENT_SWIZZLE_IDENTITY,
        .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .subresourceRange.baseMipLevel = 0,
        .subresourceRange.levelCount = 1,
        .subresourceRange.baseArrayLayer = 0,
        .subresourceRange.layerCount = 1,
        };

        if (vkCreateImageView(device, &createInfo, NULL, &swapChainImageViews[i]) != VK_SUCCESS) {
            printf("failed to create image views!");
        }
    }
}

void createRenderPass() {
    VkAttachmentDescription colorAttachment = {
        .format = selectedSurfaceFormat.format,
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
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
    VkRenderPassCreateInfo renderPassInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &colorAttachment,
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = &dependency,
    };

    if (vkCreateRenderPass(device, &renderPassInfo, NULL, &renderPass) != VK_SUCCESS) {
        printf("Error creating the render pass!");
        exit(0);
    }

}

void createFrameBuffers(){
    swapChainFramebuffersCount = swapChainImageViewsCount;
    swapChainFramebuffers = malloc(sizeof(VkFramebuffer) * swapChainImageViewsCount);

    for (uint32_t i = 0; i < swapChainImageViewsCount; i++) {
        VkImageView attachments[] = {
            swapChainImageViews[i]
        };

        VkFramebufferCreateInfo framebufferInfo = {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = renderPass,
            .attachmentCount = 1,
            .pAttachments = attachments,
            .width = swapChainExtent.width,
            .height = swapChainExtent.height,
            .layers = 1,
        };


        if (vkCreateFramebuffer(device, &framebufferInfo, NULL, &swapChainFramebuffers[i]) != VK_SUCCESS) {
            printf("Error en la creación del framebuffer con la ImageView [%d]", i);
        }
    }

}

VkShaderModule createShaderModule(char** code, size_t codeSize) {
    VkShaderModuleCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = codeSize,
        .pCode = (uint32_t*) *code,
    };
    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, NULL, &shaderModule) != VK_SUCCESS) {
        printf("failed to create shader module!");
        exit(0);
    }
    return shaderModule;
}





void createGraphicsPipeline() {
    char** triangleVert = malloc(sizeof(char*));
    char** triangleFrag = malloc(sizeof(char*));
    size_t triangleVertSize = 0;
    size_t triangleFragSize = 0;

    assert(readSPVBinaryFile("shaders/bin/triangleVert.spv\0", triangleVert, &triangleVertSize) == 0);
    assert(readSPVBinaryFile("shaders/bin/triangleFrag.spv\0", triangleFrag, &triangleFragSize) == 0);


    vertShaderModule = createShaderModule(triangleVert, triangleVertSize);
    fragShaderModule = createShaderModule(triangleFrag, triangleFragSize);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
        .module = vertShaderModule,
        .pName = "main",
    };

    VkPipelineShaderStageCreateInfo fragShaderStageInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        .module = fragShaderModule,
        .pName = "main",
    };

    shaderStages[0] = fragShaderStageInfo;
    shaderStages[1] = vertShaderStageInfo;

    VkDynamicState dynamicStates[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    uint32_t dynamicStateSize = 2;

    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = dynamicStateSize;
    dynamicState.pDynamicStates = dynamicStates;


    // VERTEX INPUT

    fillObjectInputAttributeDescription();
    fillObjectBindingDescriptions();

    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &objectIBD;
    vertexInputInfo.vertexAttributeDescriptionCount = objectsIADCount;
    vertexInputInfo.pVertexAttributeDescriptions = objectsIAD;



    /*

    void createGeometryBuffer(GEOM_OBJECT* objects){

    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = sizeof( *(objects->data)) * objects->dataLen;
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;

    if (vkCreateBuffer(device, &bufferInfo, NULL, &geometryBuffer) != VK_SUCCESS) {
        printf("failed to create vertex buffer!");
        exit(1);
    }
    void createVkBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkSharingMode sharingMode, VkMemoryPropertyFlags properties, VkBuffer *buffer, VkDeviceMemory *bufferMemory) {


};    */

    // Buffer with geometry data


    // createGeometryBuffer(&objects);
    //VkMemoryRequirements memRequirements;
    //vkGetBufferMemoryRequirements(device, geometryBuffer, &memRequirements);

    /*VkMemoryAllocateInfo allocInfo;
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    allocInfo.pNext = NULL;

    if (vkAllocateMemory(device, &allocInfo, NULL, &geometryBufferMemory) != VK_SUCCESS) {
        printf("failed to allocate vertex buffer memory!\n");
    }

    vkBindBufferMemory(device, geometryBuffer, geometryBufferMemory, 0);*/

    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport = {
        .x = 0.0f,
        .y = 0.0f,
        .width = (float) swapChainExtent.width,
        .height = (float) swapChainExtent.height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };

    VkRect2D scissor = {
        .offset = {0, 0},
        .extent = swapChainExtent,
    };

    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;


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

    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading = 1.0f; // Optional
    multisampling.pSampleMask = NULL; // Optional
    multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
    multisampling.alphaToOneEnable = VK_FALSE; // Optional


    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

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


    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, NULL, &pipelineLayout) != VK_SUCCESS) {
        printf("failed to create pipeline layout!");
        exit(1);
    }

    createRenderPass();

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
        .layout = pipelineLayout,
        .renderPass = renderPass,
        .subpass = 0,
        .basePipelineHandle = VK_NULL_HANDLE, // Optional
        .basePipelineIndex = -1, // Optional
    };

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &graphicsPipeline) != VK_SUCCESS) {
        printf("Error al crear el pipeline grafico");
        exit(1);
    }

}

// Seleccionamos de entre los formatos de superficie extraidos de las propiedades de la gpu, y seleccionamos
// el formato que soporte colores con canal alfa y componentes de 8 bits y el espacio de color SRGB
VkSurfaceFormatKHR chooseSwapSurfaceFormat(VkSurfaceFormatKHR* availableFormats, uint8_t availableFormatsCount) {
    for (uint8_t i=0; i < availableFormatsCount; i++) {
        if (availableFormats[i].format == VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT && availableFormats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormats[i];
        }
    }

    return availableFormats[0];
}

VkPresentModeKHR chooseSwapPresentMode(VkPresentModeKHR** availablePresentModes, uint8_t availableModesCount) {
    for (uint8_t i=0; i < availableModesCount; i++) {
        if (*availablePresentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
            return *availablePresentModes[i];
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR* capabilities, SDL_Window* window) {
/*    if (capabilities->currentExtent.width != UINT32_MAX) {
        //TODO add logs to all the process of setting up the vulkan instance for use
        return capabilities->currentExtent;
    } else {*/
        int width, height;
        SDL_Vulkan_GetDrawableSize(window,&width, &height);
        actualExtent.width = width;
        actualExtent.height = height;

        actualExtent.width = CLAMP(actualExtent.width, capabilities->minImageExtent.width, capabilities->maxImageExtent.width);
        actualExtent.height = CLAMP(actualExtent.height, capabilities->minImageExtent.height, capabilities->maxImageExtent.height);

        return actualExtent;
    //}
}

void createSwapChain(SDL_Window* window){
    SwapChainSupportDetails swapChainSupportDetails;

    VkResult queryPhysicalDeviceSurfaceCapabilitiesResult = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &swapChainSupportDetails.capabilities);
    assert(queryPhysicalDeviceSurfaceCapabilitiesResult == VK_SUCCESS);

    // Query de los formatos de presentacion en superficie que ofrece la gpu (physicalDevice)
    uint32_t formatCount;
    VkResult queryPhysicalDeviceSurfaceFormatsCountResult = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, NULL);
    assert(formatCount > 0 && queryPhysicalDeviceSurfaceFormatsCountResult == VK_SUCCESS);


    if (formatCount > 0) {
        swapChainSupportDetails.formats = malloc(sizeof(VkSurfaceFormatKHR)* formatCount);
        VkResult queryPhysicalDeviceSurfaceFormatsResult = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, swapChainSupportDetails.formats);
        assert(queryPhysicalDeviceSurfaceFormatsResult == VK_SUCCESS);
    }


    uint32_t presentModeCount  = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, NULL);

    // Query de
    if (presentModeCount != 0) {
        swapChainSupportDetails.presentModes = malloc(sizeof(VkSurfaceFormatKHR)* formatCount);

        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, swapChainSupportDetails.presentModes);
    }


    // Seleccionamos el formatos de superficie entre los disponibles que ofrece la gpu
    selectedSurfaceFormat = chooseSwapSurfaceFormat(swapChainSupportDetails.formats, formatCount);

    //Seleccionamos el modo de presentación para las imagenes a renderizar por la gpu

    VkPresentModeKHR selectedPresentationMode =  chooseSwapPresentMode(&swapChainSupportDetails.presentModes,presentModeCount);

    //Procesamos la resolución soportada por nuestra ventana y la del framebuffer de swapchain
    swapChainExtent = chooseSwapExtent(&swapChainSupportDetails.capabilities, window);

    // Number of images allowed in the swapchain at the same time
    swapChainImageCount = swapChainSupportDetails.capabilities.minImageCount + 1;

    //Check if imageCount exceeds the maxImageCount of the physical device surface capabilities
    if (swapChainSupportDetails.capabilities.maxImageCount > 0 && swapChainImageCount > swapChainSupportDetails.capabilities.maxImageCount) {
        swapChainImageCount = swapChainSupportDetails.capabilities.maxImageCount;
    }

    // Finally, we build the SwapChain info struct

    VkSwapchainCreateInfoKHR creationSwapChainInfo= {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = surface,
        .minImageCount = swapChainImageCount,
        .imageFormat = selectedSurfaceFormat.format,
        .imageColorSpace =selectedSurfaceFormat.colorSpace,
        .imageExtent = swapChainExtent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, // Para renderizar directamente la imagen
        .imageSharingMode = VK_SHARING_MODE_CONCURRENT,
        .queueFamilyIndexCount = queuesCreateInfoCount,
        .pQueueFamilyIndices = &queuesFamilyIndexes[0],
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = selectedPresentationMode,
        .clipped = VK_TRUE,
        .oldSwapchain = VK_NULL_HANDLE,
        .preTransform = swapChainSupportDetails.capabilities.currentTransform,
    };

    //TODO fix procesamiento de seleccion de colas, se deja hardcodeado que vamos a usar dos colas distintas 0 presentacion 1 drawing
    VkResult createSwapchainResult = vkCreateSwapchainKHR(device, &creationSwapChainInfo, NULL, &swapChain);
    if (createSwapchainResult != VK_SUCCESS) {
        printf("%u", createSwapchainResult);
        printf("failed to create swap chain!\n");
        exit(1);
    }

    vkGetSwapchainImagesKHR(device, swapChain, &swapChainImageCount, NULL);
    swapChainImages = malloc(sizeof(VkImage)* swapChainImageCount);
    if(vkGetSwapchainImagesKHR(device, swapChain, &swapChainImageCount, swapChainImages) != VK_SUCCESS){;
        printf("failed to retrieve images from the swap chain!\n");
        exit(1);
    }

}

static VKAPI_ATTR VkBool32 VKAPI_CALL vulkanDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT type, const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void * pUserData){

    printf("@@ DEBUG CALLBACK @@\n");
    printf("Mascara de severidad de mensaje: %u\n",severity);
    printf("Mascara de tipo de mensaje: %u\n",type);
    printf("Numero de objetos reportados: %d\n", pCallbackData->objectCount);
    for(uint32_t i =0; i <= pCallbackData->objectCount; i++) {
        printf("%s\n",pCallbackData->pMessage);
        printf("%lu\n",pCallbackData->pObjects[i].objectHandle);

    }
    printf("@@ DEBUG CALLBACK @@\n");

    // More deep checks and analysis here

    // Based on the data received, choose to halt or not halt program
    return VK_FALSE;

};



void destroyVulkanSurface(const VkInstance* instance, VkSurfaceKHR* surface){
    PFN_vkDestroySurfaceKHR vkDestroySurface = VK_NULL_HANDLE;
    vkDestroySurface = (PFN_vkDestroySurfaceKHR) vkGetInstanceProcAddr(*instance, "vkDestroySurfaceKHR\0");
    assert(vkDestroySurface != NULL);

    vkDestroySurface(*instance, *surface, NULL);

    printf("Vulkan surface destroyed.\n");
}

void destroyVulkanInstance(const VkInstance* instance){
    vkDestroyInstance(*instance, NULL);

    printf("Vulkan instance destroyed.\n");
}

VkDebugUtilsMessengerEXT* createDebugCallbackForVulkan(const VkInstance* instance){
    VkDebugUtilsMessengerCreateInfoEXT debug_callback_info = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .pNext = NULL,
        .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT,
        .messageType =  VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfnUserCallback = &vulkanDebugCallback,
        .pUserData = NULL,
    };
    VkDebugUtilsMessengerEXT* callback_messenger = NULL;
    callback_messenger = malloc(sizeof(VkDebugUtilsMessengerEXT));
    assert(callback_messenger != NULL);

    PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT = VK_NULL_HANDLE;
    vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(*instance, "vkCreateDebugUtilsMessengerEXT\0");
    assert(vkCreateDebugUtilsMessengerEXT != NULL);

    VkResult messenger_creation_result = vkCreateDebugUtilsMessengerEXT(*instance, &debug_callback_info, NULL,  callback_messenger);
    assert(messenger_creation_result == VK_SUCCESS);

    return callback_messenger;
}




void createCommandPool() {

    VkCommandPoolCreateInfo poolInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = queuesIndices.graphicsFamily,
    };

    if (vkCreateCommandPool(device, &poolInfo, NULL, &graphicsCommandPool) != VK_SUCCESS) {
        printf("Error al crear el command pool");
        exit(0);
    }
}

void createTransferQueueCommandPool() {
    VkCommandPoolCreateInfo transferPoolInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
        .queueFamilyIndex = queuesIndices.transferFamily,
    };

    if (vkCreateCommandPool(device, &transferPoolInfo, NULL, &transferCommandPool) != VK_SUCCESS) {
        printf("Error al crear el command pool");
        exit(0);
    }
}

void cleanupSwapChain() {
    /*vkDestroyShaderModule(device, vertShaderModule, NULL);
    vkDestroyShaderModule(device, fragShaderModule, NULL);*/
    for (size_t i = 0; i < swapChainFramebuffersCount; i++) {
        vkDestroyFramebuffer(device, swapChainFramebuffers[i], NULL);
    }

    for (size_t i = 0; i < swapChainImageViewsCount; i++) {
        vkDestroyImageView(device, swapChainImageViews[i], NULL);
    }

    vkDestroySwapchainKHR(device, swapChain, NULL);
}

void cleanup() {
    vkDeviceWaitIdle(device);
    cleanupSwapChain();
    vkDestroyBuffer(device, geometryBuffer, NULL);
    vkDestroyBuffer(device, indexBuffer, NULL);
    vkFreeMemory(device, geometryBufferMemory, NULL);
    vkFreeMemory(device, indexBufferMemory, NULL);
    vkDestroyRenderPass(device, renderPass, NULL);
    vkDestroyCommandPool(device, graphicsCommandPool, NULL);
    vkDestroyCommandPool(device, transferCommandPool, NULL);
    vkDestroyPipeline(device, graphicsPipeline, NULL);
    vkDestroyDescriptorSetLayout(device, descriptorSetLayout, NULL);
    vkDestroyPipelineLayout(device, pipelineLayout, NULL);
    cleanupSyncObjects();

    //vkDestroyPipelineLayout(device, pipelineLayout, NULL);


    vkDestroyDevice(device, NULL);

    if (logCallback) {
        destroyDebugCallbackForVulkan(&instance, logCallback);
    }

    vkDestroySurfaceKHR(instance, surface, NULL);
    vkDestroyInstance(instance, NULL);
}


void recreateSwapChain() {
    vkDeviceWaitIdle(device);
    cleanupSwapChain();
    createSwapChain(sdlWindow);
    createImageViews();
    createFrameBuffers();

}

void drawFrame() {
    vkWaitForFences(device, 1, &inFlightFence[currentFrame], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    VkResult nextImageResult = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphore[currentFrame], VK_NULL_HANDLE, &imageIndex);
    switch(nextImageResult){
        case VK_SUCCESS:
            break;
        case VK_ERROR_OUT_OF_DATE_KHR:
            recreateSwapChain();
            return;
        default:
            printf("Error a la hora de leer la siguiente imagen del dispositivo");
            exit(1);
    }
    vkResetFences(device, 1, &inFlightFence[currentFrame]);

    if( nextImageResult != VK_SUCCESS){

        printf("FALLA A LA HORA DE ADQUIRIR LA SIGUIENTE IMAGEN\n");
        exit(1);
    }

    recordCommandBuffer(commandBuffers[currentFrame], imageIndex);
    // 4. End Command Buffer Recording
    if (vkEndCommandBuffer(commandBuffers[currentFrame]) != VK_SUCCESS) {
        printf("failed to record command buffer!");
    }
    VkSemaphore waitSemaphores[] = {imageAvailableSemaphore[currentFrame]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSemaphore signalSemaphores[] = {renderFinishedSemaphore[currentFrame]};

    VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = waitSemaphores,
        .pWaitDstStageMask = waitStages,
        .commandBufferCount = 1,
        .pCommandBuffers = &commandBuffers[currentFrame],
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = signalSemaphores,
    };

    if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFence[currentFrame]) != VK_SUCCESS) {
        printf("failed to submit draw command buffer!");
    }

    VkSwapchainKHR swapChains[] = {swapChain};

    VkPresentInfoKHR presentInfo = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = signalSemaphores,
        .swapchainCount = 1,
        .pSwapchains = swapChains,
        .pImageIndices = &imageIndex,
        .pResults = NULL,
    };

    vkQueuePresentKHR(presentQueue, &presentInfo);
    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
};

void createInstance(VkInstance* instance, SDL_Window* window) {
    const VkApplicationInfo app = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = NULL,
        .pApplicationName = "GrassBox\0",
        .applicationVersion = 0,
        .pEngineName = "GrassBox\0",
        .engineVersion = 0,
        .apiVersion = VK_API_VERSION_1_0,
    };

    char* extensionsToSelect[] = {"VK_EXT_debug_utils\0", "VK_KHR_xlib_surface\0", "VK_KHR_surface\0"};

    uint32_t available_instance_layers_count = 0;
    VkLayerProperties* vulkan_instance_layer_properties = NULL;

    VkResult vkEnumInstLayerPropsResult = vkEnumerateInstanceLayerProperties(&available_instance_layers_count, NULL);
    assert(vkEnumInstLayerPropsResult == VK_SUCCESS && available_instance_layers_count > 0);
    vulkan_instance_layer_properties = malloc(sizeof(VkLayerProperties) * available_instance_layers_count);

    VkResult vkEnumInstLayPropResult = vkEnumerateInstanceLayerProperties(&available_instance_layers_count, vulkan_instance_layer_properties);
    assert(vkEnumInstLayPropResult == VK_SUCCESS);
    // Devices TODO: Aqui deberiamos buscar por las extensiones del dipositivo despues de la creación, asegurarnos
    // que soporta, y a
    char* layers_to_select[] = {"VK_LAYER_KHRONOS_validation\0"};
    const char** actual_layers_selected = malloc(sizeof(char*)*2);
    assert(actual_layers_selected != NULL);

    uint32_t actual_number_layers_selected = 0;
    for(short i = 0; i < available_instance_layers_count; i++){
        // TODO: Mirar la forma de crear una macro que incluya ARE_STRING_EQUALS y el paso de condiciones de la forma OR y AND
        if(ARE_STRINGS_EQUAL(vulkan_instance_layer_properties[i].layerName, layers_to_select[0])){
            actual_layers_selected[0] = vulkan_instance_layer_properties[i].layerName;
            actual_number_layers_selected++;
        }
    }


    //Probamos con la configuracion de SDL

    uint32_t extensionCount = 0;
    SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, NULL);
    const char** extensionNames = malloc(extensionCount * sizeof(char*));
    SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, extensionNames);

    //VkInstance instance;
    VkInstanceCreateInfo instance_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &app,
        .enabledLayerCount = actual_number_layers_selected,
        .ppEnabledLayerNames = (const char *const *)actual_layers_selected,
        .enabledExtensionCount = 3,
        .ppEnabledExtensionNames = (const char *const *) extensionsToSelect,
    };
    //TODO: Para asegurar portabilidad entre sistemas con sdl, leer la direccion de la propia funcion vkGetInstanceProdAddr
    //a traves de SDL en vez de llamarla directamente como hacemos en createDebugCallbackForVulkan

    if (vkCreateInstance(&instance_info, NULL, instance) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create Vulkan instance\n");
        exit(EXIT_FAILURE);
    }

    assert(instance != NULL);

    VkBool32 presentSupport = VK_FALSE;
    uint16_t presentSupportQueueIndex = 0;
    SDL_bool result_surface_creation = SDL_Vulkan_CreateSurface(window,
                                  *instance,
                                  &surface);
    assert(result_surface_creation == SDL_TRUE);

    PhysicalDeviceMemoryManager* pdmm = initDeviceManager();
    /*uint32_t supported_gpu_count = 0;
    vkEnumeratePhysicalDevices(*instance, &supported_gpu_count, NULL);
    assert(supported_gpu_count == 1);
    VkResult vkEnumPhysDevcResult = vkEnumeratePhysicalDevices(*instance, &supported_gpu_count, &physicalDevice);
    assert(vkEnumPhysDevcResult == VK_SUCCESS);*/




    // Queues
//    uint32_t queueFamilyCount = 0;
//    VkQueueFamilyProperties* queueFamilyProperties = VK_NULL_HANDLE;
/*    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, NULL);
    assert(queueFamilyCount > 0);

    queueFamilyProperties = malloc(sizeof(VkQueueFamilyProperties)*queueFamilyCount);

    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilyProperties);
    assert(queueFamilyCount != VK_NULL_HANDLE);*/

    // CREAMOS LA SURFACE Y BUSCAMOS UNA COLA QUE TENGA SOPORTE PARA OPERACIONES DE PRESENTACION


    // Llamamos a la funcion que selecciona las colas que vamos a usar

    getMatchingQueueIndices(pdmm, queueFamilyCount, &queuesIndices);
    //vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, presentSupportQueueIndex, surface, &presentSupport);
    //assert(presentSupport != VK_FALSE);

    /*uint32_t queueFamilyIndex = -1; //Cola operaciones graficas (la seleccion es arbitraria TODO moverlo a su sitio indicado)
    for (int i = 0; i < queueFamilyCount; i++) {
        if (queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                queueFamilyIndex = i;
        }
    }

    DEPRECATED HEMOS MOVIDO TODA LA SELECCION DE COLAS A getMatchingQUeuesIndices
    */

    //graphicsQueueIndex =  queuesIndices.graphicsFamily;


    // Creamos la cola por defecto TODO: sacar todo el tema de inicializaciond de las colas fuera, aplicar selecciones
    // basadas en algunas features y extensiones disponibles

    float_t queuePriority = 1.0f;
    //Creacion de cola para operaciones graficas
    VkDeviceQueueCreateInfo queueCreateInfoGraphics = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = queuesIndices.graphicsFamily,
        .queueCount = 1,
        .pQueuePriorities = &queuePriority,
        .pNext = NULL,
    };

    VkDeviceQueueCreateInfo queueCreateInfoPresentation = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = queuesIndices.presentFamily,
        .queueCount = 1,
        .pQueuePriorities = &queuePriority,
        .pNext = NULL,
    };

    VkDeviceQueueCreateInfo queueCreateInfoTransfer = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = queuesIndices.transferFamily,
        .queueCount = 1,
        .pQueuePriorities = &queuePriority,
        .pNext = NULL,
    };

    queuesCreateInfo = malloc(sizeof(VkDeviceQueueCreateInfo) * 2);
    //queuesCreateInfo[0] = queueCreateInfoPresentation;
    queuesCreateInfo[0] = queueCreateInfoGraphics;
    queuesCreateInfo[1] = queueCreateInfoTransfer;

    VkPhysicalDeviceFeatures deviceFeatures = {VK_FALSE};
    // TODO: moverlo a una funcion o fichero para configuracion de dispositivo y dispositivo logico
    const char* logicalDeviceExtensionNames = "VK_KHR_swapchain\0";
    //Logical devicce
    VkDeviceCreateInfo createLogicalDeviceInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pQueueCreateInfos = queuesCreateInfo,
        .queueCreateInfoCount = 2,
        .pEnabledFeatures = &deviceFeatures,
        .ppEnabledExtensionNames = &logicalDeviceExtensionNames,
        .enabledExtensionCount = 1,
        .pNext = NULL,


    };

    if (vkCreateDevice(physicalDevice, &createLogicalDeviceInfo, NULL, &device) != VK_SUCCESS) {
        printf("failed to create logical device!");
        exit(1);
    }

    vkGetDeviceQueue(device, queuesIndices.graphicsFamily, 0, &graphicsQueue);
    vkGetDeviceQueue(device, queuesIndices.presentFamily, 0, &presentQueue);
    vkGetDeviceQueue(device, queuesIndices.transferFamily, 0, &transferQueue);



}

void fillGeometryData(){
    objects.dataLen = 4;
    objects.indicesLen = 6;
    objects.data = malloc(sizeof(*(objects.data)) * objects.dataLen);

    GEOM_VERTEX_COLOR_RGBA colorData[4];
    for(uint32_t i=0; i < 4; i++){
            colorData[i].r = i == 0 ? 1.0f : i == 1 ? 0.0f : i == 2 ? 0.0f : 1.0f;
            colorData[i].b = i == 0 ? 0.0f : i == 1 ? 1.0f : i == 2 ? 0.0f : 1.0f;
            colorData[i].g = i == 0 ? 0.0f : i == 1 ? 0.0f : i == 2 ? 1.0f : 1.0f;
            colorData[i].a = 1.0f;
            objects.data[i].color = colorData[i];
    }

    GEOM_VERTEX_POSITION positionDat[4];
    for(uint32_t i=0; i < 4; i++){
            positionDat[i].x = i == 0 ? -0.5f : i == 1 ? 0.5f : i == 2 ? 0.5f : -0.5f;
            positionDat[i].y = i == 0 ? -0.5f : i == 1 ? -0.5f : i == 2 ? 0.5f : 0.5f;
            positionDat[i].z = 0.0f;
            printf("Punto %d (%f,%f,%f)\n", i, positionDat[i].x, positionDat[i].y, positionDat[i].z);
            objects.data[i].point = positionDat[i];
    }

    objects.indices = malloc(sizeof(uint32_t) * objects.indicesLen);
    objects.indices[0] = 0;
    objects.indices[1] = 1;
    objects.indices[2] = 2;
    objects.indices[3] = 2;
    objects.indices[4] = 3;
    objects.indices[5] = 0;
}

void initVulkan(SDL_Window* window){
    sdlWindow = window;
    exit(1);
    fillGeometryData();
    createInstance(&instance, window);
    createSwapChain(window);
    createImageViews();
    createGraphicsPipeline();
    createFrameBuffers();
    createCommandPool();
    createTransferQueueCommandPool();
    createCommandBuffer();
    createSyncObjects();
    createGeometryBuffer(&objects);
    createIndiceBuffer(&objects);
    logCallback = createDebugCallbackForVulkan(&instance);
    printf("Init vulkan ended.\n");

}
