#pragma once
#include <vulkan/vulkan.h>
#include <stdbool.h>
#include <stdint.h>
#include "vma_bridge.h"

// Initial capacity for each per-type array in a slot.
// Grows via realloc if exceeded — zero allocations at steady state.
#define DELETION_QUEUE_INITIAL_CAPACITY 16

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

// Ring buffer of deletion slots, one per in-flight frame (slotCount equals
// ld->frameSync.maxFramesInFlight). Resources are deferred
// for slotCount frames before destruction, matching the GPU's pipeline depth.
// currentSlot is derived from the frame number on every flush:
//   currentSlot = graphicsWorkloadRun % slotCount
// The flush destroys what frame (run - slotCount) queued in that slot, then this
// frame's deferDestroy* calls fill the same slot.
typedef struct DeletionQueue
{
    DeletionQueueSlot* slots;
    uint32_t           slotCount;
    uint32_t           currentSlot; // graphicsWorkloadRun % slotCount, set by flushDeletionQueue
    uint64_t           lastFlushedRun;
} DeletionQueue;

// Allocate slotCount slots with starting capacity. Call once after device
// creation, passing ld->frameSync.maxFramesInFlight.
void initDeletionQueue(DeletionQueue* queue, uint32_t slotCount);

// Set currentSlot = frameRun % slotCount and destroy all resources in it.
// frameRun is the graphics timeline's graphicsWorkloadRun for this frame.
// Must be called AFTER the timeline wait confirms frame (frameRun - slotCount)
// finished, and before this frame's deferDestroy* calls.
void flushDeletionQueue(DeletionQueue* queue, uint64_t frameRun, VkDevice device, VmaAllocator allocator);

// Flush ALL slots and free the queue's internal arrays. Call at shutdown
// after vkDeviceWaitIdle, before destroying the VMA allocator.
void destroyDeletionQueue(DeletionQueue* queue, VkDevice device, VmaAllocator allocator);

bool deletionQueueIsEmpty(const DeletionQueue* queue);

// --- Enqueue functions ---
// Each pushes a resource into the current slot for deferred destruction.

void deferDestroyBuffer(DeletionQueue* queue, VkBuffer buffer, VmaAllocation allocation);
void deferDestroyImage(DeletionQueue* queue, VkImage image, VmaAllocation allocation);
void deferDestroyImageView(DeletionQueue* queue, VkImageView imageView);
