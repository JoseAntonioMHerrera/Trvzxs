// VMA Bridge — C++ implementation of the C-facing wrapper functions.
//
// This file includes <vk_mem_alloc.h> (C++ header) and implements the
// extern "C" functions declared in vma_bridge.h. The C engine calls these;
// they forward to VMA's actual API.

#include <vk_mem_alloc.h>
#include "vma_bridge.h"
#include <cstdio>

extern "C"
{

    // --- Allocator lifecycle ---

    VkResult vmaBridgeCreateAllocator(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device,
                                      VmaAllocator* outAllocator)
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
        }
        else
        {
        }
        return result;
    }

    void vmaBridgeDestroyAllocator(VmaAllocator allocator)
    {
        if (allocator != VK_NULL_HANDLE)
        {
            vmaDestroyAllocator(allocator);
        }
    }

    // --- Buffer operations ---

    VkResult vmaBridgeCreateBuffer(VmaAllocator allocator, VkDeviceSize size, VkBufferUsageFlags usage,
                                   VkSharingMode sharingMode, VmaBridgeMemoryUsage memUsage, VkBuffer* outBuffer,
                                   VmaAllocation* outAllocation)
    {
        VkBufferCreateInfo bufferInfo = {};
        bufferInfo.sType              = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size               = size;
        bufferInfo.usage              = usage;
        bufferInfo.sharingMode        = sharingMode;

        VmaAllocationCreateInfo allocInfo = {};
        switch (memUsage)
        {
        case VMA_BRIDGE_MEMORY_GPU_ONLY:
            allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
            break;
        case VMA_BRIDGE_MEMORY_CPU_TO_GPU:
            allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
            allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
            break;
        }

        return vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, outBuffer, outAllocation, nullptr);
    }

    VkResult vmaBridgeCreateBufferMapped(VmaAllocator allocator, VkDeviceSize size, VkBufferUsageFlags usage,
                                         VkSharingMode sharingMode, VkBuffer* outBuffer, VmaAllocation* outAllocation,
                                         void** outMappedPtr)
    {
        VkBufferCreateInfo bufferInfo = {};
        bufferInfo.sType              = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size               = size;
        bufferInfo.usage              = usage;
        bufferInfo.sharingMode        = sharingMode;

        VmaAllocationCreateInfo allocInfo = {};
        allocInfo.usage                   = VMA_MEMORY_USAGE_AUTO;
        allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

        VmaAllocationInfo allocationInfo = {};
        VkResult          result =
            vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, outBuffer, outAllocation, &allocationInfo);
        if (result == VK_SUCCESS && outMappedPtr)
        {
            *outMappedPtr = allocationInfo.pMappedData;
        }
        return result;
    }

    void vmaBridgeDestroyBuffer(VmaAllocator allocator, VkBuffer buffer, VmaAllocation allocation)
    {
        vmaDestroyBuffer(allocator, buffer, allocation);
    }

    VkResult vmaBridgeMapMemory(VmaAllocator allocator, VmaAllocation allocation, void** outData)
    {
        return vmaMapMemory(allocator, allocation, outData);
    }

    void vmaBridgeUnmapMemory(VmaAllocator allocator, VmaAllocation allocation)
    {
        vmaUnmapMemory(allocator, allocation);
    }

    // --- Image operations ---

    VkResult vmaBridgeCreateImage(VmaAllocator              allocator,
                                  const VkImageCreateInfo*  imageInfo,
                                  VmaBridgeMemoryUsage      memUsage,
                                  VkImage*                  outImage,
                                  VmaAllocation*            outAllocation)
    {
        VmaAllocationCreateInfo allocInfo = {};
        switch (memUsage)
        {
        case VMA_BRIDGE_MEMORY_GPU_ONLY:
            allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
            break;
        case VMA_BRIDGE_MEMORY_CPU_TO_GPU:
            allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
            allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
            break;
        }

        return vmaCreateImage(allocator, imageInfo, &allocInfo, outImage, outAllocation, nullptr);
    }

    void vmaBridgeDestroyImage(VmaAllocator allocator, VkImage image, VmaAllocation allocation)
    {
        vmaDestroyImage(allocator, image, allocation);
    }

} // extern "C"
