#pragma once
#include <vulkan/vulkan.h>
#include <stdint.h>
#include "vma_bridge.h"

// Initial capacity for each per-type array in a slot.
// Grows via realloc if exceeded — zero allocations at steady state.
#define DELETION_QUEUE_INITIAL_CAPACITY 16

// Maximum number of in-flight frames. Determines how many slots the
// ring buffer has — resources are deferred for this many frames before
// destruction, matching the GPU's pipeline depth.
#ifndef MAX_FRAMES_IN_FLIGHT
    #define MAX_FRAMES_IN_FLIGHT 2
#endif

// One slot per frame in the ring buffer. Each slot holds per-type arrays
// of resources enqueued for deferred destruction during that frame.
// Flush order is hardcoded: image views → buffers → images.
// This guarantees children are destroyed before parents.
typedef struct DeletionQueueSlot
{
    // VMA-managed buffers (VkBuffer + VmaAllocation pairs)
    VkBuffer*      buffers;
    VmaAllocation* bufferAllocs;
    uint32_t       bufferCount;
    uint32_t       bufferCapacity;

    // VMA-managed images (VkImage + VmaAllocation pairs)
    VkImage*       images;
    VmaAllocation* imageAllocs;
    uint32_t       imageCount;
    uint32_t       imageCapacity;

    // Image views (no allocation — just a VkImageView handle)
    VkImageView*   imageViews;
    uint32_t       imageViewCount;
    uint32_t       imageViewCapacity;

} DeletionQueueSlot;

// Ring buffer of deletion slots, one per in-flight frame.
// Enqueues go into slots[currentSlot]. Flush destroys everything in
// slots[currentSlot] (the oldest frame), then advance moves to the next slot.
typedef struct DeletionQueue
{
    DeletionQueueSlot slots[MAX_FRAMES_IN_FLIGHT];
    uint32_t          currentSlot;
} DeletionQueue;

// Initialize all slots with starting capacity. Call once after device creation.
void initDeletionQueue(DeletionQueue* queue);

// Flush the current slot — destroy all resources in it.
// Must be called AFTER vkWaitForFences confirms the GPU finished with
// the commands that referenced these resources.
void flushDeletionQueue(DeletionQueue* queue, VkDevice device, VmaAllocator allocator);

// Advance to the next ring buffer slot. Call once per frame after flushing.
void advanceDeletionQueue(DeletionQueue* queue);

// Flush ALL slots and free the queue's internal arrays. Call at shutdown
// after vkDeviceWaitIdle, before destroying the VMA allocator.
void destroyDeletionQueue(DeletionQueue* queue, VkDevice device, VmaAllocator allocator);

// --- Enqueue functions ---
// Each pushes a resource into the current slot for deferred destruction.

void deferDestroyBuffer(DeletionQueue* queue, VkBuffer buffer, VmaAllocation allocation);
void deferDestroyImage(DeletionQueue* queue, VkImage image, VmaAllocation allocation);
void deferDestroyImageView(DeletionQueue* queue, VkImageView imageView);
