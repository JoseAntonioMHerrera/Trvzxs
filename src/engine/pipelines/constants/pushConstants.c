#include <vulkan/vulkan.h>
#include "monotonicTime.h"
#include "pushConstants.h"

void updatePushConstants(VkCommandBuffer commandBuffer, VkPipelineLayout *pipelineLayout) {
    static struct timespec startTime;
    static int initialized = 0;
    struct timespec currentTime;

    if (!initialized) {
        getMonotonicTime(&startTime);
        initialized = 1;
    }

    getMonotonicTime(&currentTime);

    // Calculate delta time in seconds
    float deltaTime = (currentTime.tv_sec - startTime.tv_sec) +
                      (currentTime.tv_nsec - startTime.tv_nsec) / 1e9f;

    PushConstantDisplayTimer pushData = {
        .deltaTime = deltaTime,
    };

    vkCmdPushConstants(commandBuffer, *pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstantDisplayTimer), &pushData);

}
