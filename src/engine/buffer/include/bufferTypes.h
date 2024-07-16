#pragma once
#include <vulkan/vulkan.h>

typedef struct UniformBufferManager {
    VkBuffer *uniformBuffers;
    uint32_t uniformBuffersLen;
    VkDeviceMemory *uniformBuffersMemory;
    void** uniformBuffersMapped;
    VkDescriptorSetLayout *dsl;
    VkDescriptorPool *descriptorPool;
    VkDescriptorSet *descriptorSet;
} UniformBufferManager
;
