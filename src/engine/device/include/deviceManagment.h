#pragma once
#include "../../swapchain/include/swapchainManagerTypes.h"
#include "../../../include/langDefinitions/compilerMacros.h"
#include <stdalign.h>
// DISPLAY MANAGER //
#define COMMAND_POOL_OFFSET offsetof(DisplayManager, commandPools);
#define DEVICE_BUFFER_OFFSET offsetof(DisplayManager, deviceBuffer);

TRVZXS_inline void* fget(void* self, size_t offset ) {
    return (char*)self + offset; // char*, bytewise
}



typedef struct CommandPools {
    VkCommandPool* transferCommandPool;
    // Add other command pools as needed
} CommandPools;

typedef struct QueueManager {
    VkQueue graphicsQueue;
    VkQueue presentQueue;
    VkQueue transferQueue;
} QueueManager;

typedef struct {
    uint32_t graphicsFamily;
    uint32_t presentFamily;
    uint32_t transferFamily;
} QueueFamilyIndices;

typedef struct DeviceBufferManager {
    VkBuffer *geometryBuffer;
    VkDeviceMemory *geometryBufferMemory;
    VkBuffer *indexBuffer;
    VkDeviceMemory *indexBufferMemory;
    VkBuffer *stagingBuffer;
    VkDeviceMemory *stagingBufferMemory;
} DeviceBufferManager;

typedef struct DisplayManager {
    VkSurfaceKHR surface;
    VkFence inFlightFence;
    uint8_t alignas(alignof(uint8_t)) currentFrame;
    DisplaySwapchainManager *swapChainM;
    CommandPools *commandPools;
    DeviceBufferManager *deviceBuffer;
    void* getField;
    void* drawFrame;
} DisplayManager;


typedef struct PhysicalDevice {
    VkPhysicalDevice gpu;
    VkPhysicalDeviceMemoryProperties memProperties;
    VkQueueFamilyProperties *queueFamilyProperties;
    uint32_t queueFamilyCount;
    QueueFamilyIndices queueFamilyIndices;
} PhysicalDevice;

typedef struct LogicalDevice {
    size_t deviceLen;
    VkDevice device;
    QueueManager queueManager;
    DisplayManager *displayManager;
} LogicalDevice;

typedef struct DeviceManager {
    size_t phyDeviceLen;
    size_t deviceLen;
    PhysicalDevice *pDevice;
    LogicalDevice **lDevice;
    size_t deviceIndex;
    size_t gpuIndex;
    LogicalDevice* (*getDevice)(size_t index);
} DeviceManager;

uint32_t findMemoryType(PhysicalDevice *pd, uint32_t typeFilter, VkMemoryPropertyFlags properties) ;
void getMatchingQueueIndices(PhysicalDevice *pd, LogicalDevice* ld, VkQueueFamilyProperties* properties, int numProperties, QueueFamilyIndices *queuesIndices);
