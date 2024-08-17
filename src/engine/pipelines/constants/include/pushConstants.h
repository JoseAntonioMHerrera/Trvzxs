#pragma once

typedef struct {
    float deltaTime;
} PushConstantDisplayTimer;

void updatePushConstants(VkCommandBuffer commandBuffer, VkPipelineLayout *pipelineLayout);
