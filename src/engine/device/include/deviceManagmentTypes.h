#pragma once
#include "../../swapchain/include/swapchainManagerTypes.h"
#include "compilerMacros.h"
#include "physicalDevice.h"
#include "geometryPipelineTypes.h"
#include <stdalign.h>
#include <stddef.h>
#include "vma_bridge.h"
#include "deletionQueue.h"

typedef struct DisplayManager
{
    VkSurfaceKHR              surface;
    VkFence                   inFlightFence;
    uint32_t                  currentFrame;
    GraphicPipelineToSurface* graphicPipeline;
    uint32_t                  graphicsPipelineLen;
    // void* getField;
    // void* (*drawFrame)(uint8_t currentFrame);
} DisplayManager; // TODO: remove this struct and move it to graphicPipelineToSurface struct

typedef struct LogicalDevice
{
    size_t          deviceLen;
    VkDevice        device;
    QueueManager    queueManager;
    DisplayManager* displayManager;
    VmaAllocator    allocator;      // VMA suballocator — one per logical device, manages all GPU memory
    DeletionQueue   deletionQueue;  // Per-frame deferred destruction ring buffer
} LogicalDevice;

typedef struct DeviceManager
{
    size_t          phyDeviceLen;
    size_t          deviceLen;
    PhysicalDevice* pDevice;
    LogicalDevice** lDevice;
    size_t          deviceIndex;
    size_t          gpuIndex;
    LogicalDevice* (*getDevice)(size_t index);
    VkInstance*               instance;
    VkDebugUtilsMessengerEXT* logCallback;

} DeviceManager;
