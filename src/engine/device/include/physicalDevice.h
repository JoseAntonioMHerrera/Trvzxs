#pragma once
#include <vulkan/vulkan.h>

typedef struct {
    uint32_t graphicsFamily;
    uint32_t presentFamily;
    uint32_t transferFamily;
    uint32_t computeFamily;
} QueueFamilyIndices;

typedef struct QueueManager {
    VkQueue *graphicsQueue;
    VkQueue *presentQueue;
    VkQueue *transferQueue;
    VkQueue *computeQueue;
} QueueManager;

typedef struct PhysicalDevice {
    VkPhysicalDevice *gpu;
    VkPhysicalDeviceMemoryProperties *memProperties;
    VkPhysicalDeviceProperties gpuProperties;
    VkQueueFamilyProperties *queueFamilyProperties;
    uint32_t queueFamilyCount;

    QueueFamilyIndices queueFamilyIndices;
} PhysicalDevice;
