#pragma once
#include <vulkan/vulkan.h>
#include <stdbool.h>
#include <stdint.h>

// Maximum capacities — compile-time limits, not allocations.
#define RG_MAX_PASSES            16
#define RG_MAX_RESOURCES         32
#define RG_MAX_RESOURCE_USAGES   8
#define RG_MAX_BARRIERS_PER_PASS 8

// --- Enums ---

typedef enum RGAccessType
{
    RG_READ,
    RG_WRITE,
    RG_READ_WRITE,
} RGAccessType;

typedef enum RGSizeMode
{
    RG_SIZE_SWAPCHAIN,
    RG_SIZE_FIXED,
} RGSizeMode;

typedef enum RGResourceType
{
    RG_IMAGE,
    RG_BUFFER,
} RGResourceType;

typedef enum RGPassType
{
    RG_PASS_GRAPHICS,
    RG_PASS_COMPUTE,
    RG_PASS_TRANSFER,
} RGPassType;

typedef enum RGQueueType
{
    RG_QUEUE_GRAPHICS,
    RG_QUEUE_COMPUTE,
    RG_QUEUE_TRANSFER,
} RGQueueType;

// --- Resources ---

typedef struct RGResource
{
    RGResourceType type;

    // Image-specific (ignored for RG_BUFFER)
    VkFormat              format;
    VkSampleCountFlagBits samples;
    RGSizeMode            sizeMode;
    uint32_t              width;
    uint32_t              height;
    VkImageUsageFlags     usage;
    VkImageAspectFlags    aspectMask;

    // Resolved handles
    VkImage        image;
    VkImageView    imageView;
    VkDeviceMemory memory;

    // Per-frame image views for external resources that change per swapchain
    // image (e.g. the swapchain color attachment). NULL if the same view is
    // used for all frames. When non-NULL, framebuffers[i] uses
    // perFrameImageViews[i] instead of imageView.
    VkImageView* perFrameImageViews;

    // Buffer-specific (ignored for RG_IMAGE)
    VkBuffer     buffer;
    VkDeviceSize bufferSize;

    // Ownership
    bool isExternal;
    bool isTransient;
    bool isActive;

    // Tracking: updated during barrier computation
    VkImageLayout currentLayout;
    uint32_t      lastWriterPass; // UINT32_MAX if none
} RGResource;

// --- Resource usage (per-pass declaration) ---

typedef struct RGResourceUsage
{
    uint32_t             resourceIndex;
    VkImageLayout        layout;
    VkAccessFlags        accessMask;
    VkPipelineStageFlags stageMask;
    RGAccessType         access;

    // Set by rgComputeBarriers(), not by the user.
    // UNDEFINED = first write this frame (loadOp=CLEAR).
    // Anything else = previous pass wrote it (loadOp=LOAD).
    VkImageLayout resolvedInitialLayout;
} RGResourceUsage;

// --- Passes ---

typedef struct RGPass RGPass;

typedef void (*RGPassExecuteFn)(VkCommandBuffer cmd, const RGPass* pass, void* userData);

struct RGPass
{
    RGPassType  type;
    RGQueueType queueType;
    bool        isActive;

    RGResourceUsage inputs[RG_MAX_RESOURCE_USAGES];
    uint32_t        inputCount;
    RGResourceUsage outputs[RG_MAX_RESOURCE_USAGES];
    uint32_t        outputCount;

    RGPassExecuteFn execute;
    void*           userData;

    // Resolved at compile time
    uint32_t       executionOrder;
    VkRenderPass   renderPass;
    VkFramebuffer* framebuffers; // [swapchainImageCount]

    // Acquire barriers — emitted BEFORE this pass's callback.
    // When the writer is on a different queue, these use real queue family indices.
    VkImageMemoryBarrier  imageBarriers[RG_MAX_BARRIERS_PER_PASS];
    uint32_t              imageBarrierResourceIdx[RG_MAX_BARRIERS_PER_PASS];
    uint32_t              imageBarrierCount;
    VkBufferMemoryBarrier bufferBarriers[RG_MAX_BARRIERS_PER_PASS];
    uint32_t              bufferBarrierResourceIdx[RG_MAX_BARRIERS_PER_PASS];
    uint32_t              bufferBarrierCount;

    VkPipelineStageFlags barrierSrcStageMask;
    VkPipelineStageFlags barrierDstStageMask;

    // Release barriers — emitted AFTER this pass's callback when an output
    // is consumed by a pass on a different queue.
    VkBufferMemoryBarrier releaseBufferBarriers[RG_MAX_BARRIERS_PER_PASS];
    uint32_t              releaseBufferBarrierResourceIdx[RG_MAX_BARRIERS_PER_PASS];
    uint32_t              releaseBufferBarrierCount;
    VkImageMemoryBarrier  releaseImageBarriers[RG_MAX_BARRIERS_PER_PASS];
    uint32_t              releaseImageBarrierResourceIdx[RG_MAX_BARRIERS_PER_PASS];
    uint32_t              releaseImageBarrierCount;
    VkPipelineStageFlags  releaseSrcStageMask;
    VkPipelineStageFlags  releaseDstStageMask;
};

// --- The graph ---

typedef struct PhysicalDevice PhysicalDevice;

typedef struct RenderGraph
{
    RGResource resources[RG_MAX_RESOURCES];
    uint32_t   resourceCount;

    RGPass   passes[RG_MAX_PASSES];
    uint32_t passCount;

    uint32_t executionOrder[RG_MAX_PASSES];
    uint32_t executionOrderCount;

    VkDevice        device;
    PhysicalDevice* physicalDevice;

    uint32_t swapchainWidth;
    uint32_t swapchainHeight;
    uint32_t swapchainImageCount;

    bool isCompiled;
    bool debugLog;

    float deltaTime;

    // Multi-queue support
    VkQueue  graphicsQueue;
    VkQueue  computeQueue;
    uint32_t graphicsQueueFamily;
    uint32_t computeQueueFamily;
    bool     hasComputeWork; // set during rgExecute if any compute pass was recorded

#ifdef DEBUG
    // GPU timestamp queries — one pair (begin/end) per pass, per frame-in-flight.
    // Layout: frame f, pass p → query index = f * (passCount*2) + p*2  (begin)
    //                                                              +1  (end)
    VkQueryPool gpuTimestampPool;
    uint32_t    gpuTimestampQueryCount; // total queries allocated
    float       timestampPeriodNs;     // nanoseconds per tick
    uint32_t    gpuTimingFrames;
    double      gpuAccumUs[RG_MAX_PASSES]; // accumulated per-pass µs
#endif
} RenderGraph;
