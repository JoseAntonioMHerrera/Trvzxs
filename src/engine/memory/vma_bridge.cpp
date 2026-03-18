// VMA Bridge — C++ implementation of the C-facing wrapper functions.
//
// This file includes <vk_mem_alloc.h> (C++ header) and implements the
// extern "C" functions declared in vma_bridge.h. The C engine calls these;
// they forward to VMA's actual API.

#include <vk_mem_alloc.h>
#include "vma_bridge.h"
#include <cstdio>

extern "C" {

VkResult vmaBridgeCreateAllocator(VkInstance       instance,
                                  VkPhysicalDevice physicalDevice,
                                  VkDevice         device,
                                  VmaAllocator*    outAllocator)
{
    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.flags                  = 0;
    allocatorInfo.physicalDevice         = physicalDevice;
    allocatorInfo.device                 = device;
    allocatorInfo.instance               = instance;
    allocatorInfo.vulkanApiVersion       = VK_API_VERSION_1_3;

    VkResult result = vmaCreateAllocator(&allocatorInfo, outAllocator);
    if (result == VK_SUCCESS)
    {
        printf("VMA allocator created successfully\n");
    }
    else
    {
        printf("ERROR: Failed to create VMA allocator! VkResult: %d\n", result);
    }
    return result;
}

void vmaBridgeDestroyAllocator(VmaAllocator allocator)
{
    if (allocator != VK_NULL_HANDLE)
    {
        vmaDestroyAllocator(allocator);
        printf("VMA allocator destroyed\n");
    }
}

} // extern "C"
