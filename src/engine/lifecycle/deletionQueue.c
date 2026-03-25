#include "deletionQueue.h"
#include <stdlib.h>
#include <string.h>

// --- Internal helpers ---

static void initSlot(DeletionQueueSlot* slot)
{
    slot->bufferCapacity    = DELETION_QUEUE_INITIAL_CAPACITY;
    slot->buffers           = malloc(sizeof(VkBuffer) * slot->bufferCapacity);
    slot->bufferAllocs      = malloc(sizeof(VmaAllocation) * slot->bufferCapacity);
    slot->bufferCount       = 0;

    slot->imageCapacity     = DELETION_QUEUE_INITIAL_CAPACITY;
    slot->images            = malloc(sizeof(VkImage) * slot->imageCapacity);
    slot->imageAllocs       = malloc(sizeof(VmaAllocation) * slot->imageCapacity);
    slot->imageCount        = 0;

    slot->imageViewCapacity = DELETION_QUEUE_INITIAL_CAPACITY;
    slot->imageViews        = malloc(sizeof(VkImageView) * slot->imageViewCapacity);
    slot->imageViewCount    = 0;
}

static void flushSlot(DeletionQueueSlot* slot, VkDevice device, VmaAllocator allocator)
{
    // Destruction order: children before parents.
    // Image views reference images, so destroy views first.

    for (uint32_t i = 0; i < slot->imageViewCount; i++)
    {
        vkDestroyImageView(device, slot->imageViews[i], NULL);
    }
    slot->imageViewCount = 0;

    for (uint32_t i = 0; i < slot->bufferCount; i++)
    {
        vmaBridgeDestroyBuffer(allocator, slot->buffers[i], slot->bufferAllocs[i]);
    }
    slot->bufferCount = 0;

    for (uint32_t i = 0; i < slot->imageCount; i++)
    {
        vmaBridgeDestroyImage(allocator, slot->images[i], slot->imageAllocs[i]);
    }
    slot->imageCount = 0;
}

static void freeSlot(DeletionQueueSlot* slot)
{
    free(slot->buffers);
    free(slot->bufferAllocs);
    free(slot->images);
    free(slot->imageAllocs);
    free(slot->imageViews);

    slot->buffers      = NULL;
    slot->bufferAllocs = NULL;
    slot->images       = NULL;
    slot->imageAllocs  = NULL;
    slot->imageViews   = NULL;

    slot->bufferCount       = 0;
    slot->bufferCapacity    = 0;
    slot->imageCount        = 0;
    slot->imageCapacity     = 0;
    slot->imageViewCount    = 0;
    slot->imageViewCapacity = 0;
}

// --- Public API ---

void initDeletionQueue(DeletionQueue* queue)
{
    queue->currentSlot = 0;
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        initSlot(&queue->slots[i]);
    }
}

void flushDeletionQueue(DeletionQueue* queue, VkDevice device, VmaAllocator allocator)
{
    flushSlot(&queue->slots[queue->currentSlot], device, allocator);
}

void advanceDeletionQueue(DeletionQueue* queue)
{
    queue->currentSlot = (queue->currentSlot + 1) % MAX_FRAMES_IN_FLIGHT;
}

void destroyDeletionQueue(DeletionQueue* queue, VkDevice device, VmaAllocator allocator)
{
    // Flush every slot — at shutdown there may be entries from the last 1-2 frames.
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        flushSlot(&queue->slots[i], device, allocator);
        freeSlot(&queue->slots[i]);
    }
}

// --- Enqueue functions ---

static void growBufferArrays(DeletionQueueSlot* slot)
{
    slot->bufferCapacity *= 2;
    slot->buffers        = realloc(slot->buffers, sizeof(VkBuffer) * slot->bufferCapacity);
    slot->bufferAllocs   = realloc(slot->bufferAllocs, sizeof(VmaAllocation) * slot->bufferCapacity);
}

static void growImageArrays(DeletionQueueSlot* slot)
{
    slot->imageCapacity *= 2;
    slot->images        = realloc(slot->images, sizeof(VkImage) * slot->imageCapacity);
    slot->imageAllocs   = realloc(slot->imageAllocs, sizeof(VmaAllocation) * slot->imageCapacity);
}

static void growImageViewArray(DeletionQueueSlot* slot)
{
    slot->imageViewCapacity *= 2;
    slot->imageViews        = realloc(slot->imageViews, sizeof(VkImageView) * slot->imageViewCapacity);
}

void deferDestroyBuffer(DeletionQueue* queue, VkBuffer buffer, VmaAllocation allocation)
{
    DeletionQueueSlot* slot = &queue->slots[queue->currentSlot];
    if (slot->bufferCount >= slot->bufferCapacity)
        growBufferArrays(slot);

    slot->buffers[slot->bufferCount]      = buffer;
    slot->bufferAllocs[slot->bufferCount] = allocation;
    slot->bufferCount++;
}

void deferDestroyImage(DeletionQueue* queue, VkImage image, VmaAllocation allocation)
{
    DeletionQueueSlot* slot = &queue->slots[queue->currentSlot];
    if (slot->imageCount >= slot->imageCapacity)
        growImageArrays(slot);

    slot->images[slot->imageCount]      = image;
    slot->imageAllocs[slot->imageCount] = allocation;
    slot->imageCount++;
}

void deferDestroyImageView(DeletionQueue* queue, VkImageView imageView)
{
    DeletionQueueSlot* slot = &queue->slots[queue->currentSlot];
    if (slot->imageViewCount >= slot->imageViewCapacity)
        growImageViewArray(slot);

    slot->imageViews[slot->imageViewCount] = imageView;
    slot->imageViewCount++;
}
