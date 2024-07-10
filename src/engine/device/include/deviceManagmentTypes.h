#pragma once
#include "../../swapchain/include/swapchainManagerTypes.h"
#include "compilerMacros.h"
#include "geometryPipelineTypes.h"
#include <stdalign.h>
#include <stddef.h>
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

typedef struct DisplayManager {
    VkSurfaceKHR surface;
    VkFence inFlightFence;
    uint8_t alignas(alignof(uint8_t)) currentFrame;
    DisplaySwapchainManager *swapChainM;
    CommandPools *commandPools;
    GeometryDisplayPipelineManager *geomPipeline;
    void* (*createGeometryPipeline)();

    void* getField;
    void* (*drawFrame)(uint8_t currentFrame);
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
    VkInstance *instance;
    VkDebugUtilsMessengerEXT* logCallback;

} DeviceManager;
