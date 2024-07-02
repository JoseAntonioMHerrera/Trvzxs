#ifndef _FAMILY_QUEUES_H_
#define _FAMILY_QUEUES_H_



typedef struct QueueFamilyIndices {
    uint32_t graphicsFamily;
    uint32_t presentFamily;
    uint32_t transferFamily;
} QueueFamilyIndices;

void getMatchingQueueIndices(const VkQueueFamilyProperties* properties, int numProperties, QueueFamilyIndices *queuesIndices);

#endif
