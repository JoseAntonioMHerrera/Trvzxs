#pragma once
// C-compatible bridge to VMA (Vulkan Memory Allocator).
//
// VMA is a C++ library. This header provides opaque handle types and wrapper
// functions so the rest of the engine (pure C) can use VMA without ever
// including <vk_mem_alloc.h>.

#include <vulkan/vulkan.h>

// Opaque VMA handles — mirrors the VK_DEFINE_HANDLE declarations inside VMA.
// C code sees these as typed pointers; the actual struct definitions live in
// vk_mem_alloc.h (C++ only).
typedef struct VmaAllocator_T*  VmaAllocator;
typedef struct VmaAllocation_T* VmaAllocation;

// Simplified memory usage hints for the C engine.
// The bridge translates these to VMA's internal flags.
typedef enum VmaBridgeMemoryUsage
{
    VMA_BRIDGE_MEMORY_GPU_ONLY,   // Device-local, not host-accessible (vertex/index/SSBO on GPU)
    VMA_BRIDGE_MEMORY_CPU_TO_GPU, // Host-visible, for staging uploads and CPU-written buffers
} VmaBridgeMemoryUsage;

#ifdef __cplusplus
extern "C" {
#endif

// --- Allocator lifecycle ---

// Create a VMA allocator tied to the given Vulkan device/instance.
// Must be called after the logical device and queues are initialized.
VkResult vmaBridgeCreateAllocator(VkInstance       instance,
                                  VkPhysicalDevice physicalDevice,
                                  VkDevice         device,
                                  VmaAllocator*    outAllocator);

// Destroy a VMA allocator. Call before vkDestroyDevice.
void vmaBridgeDestroyAllocator(VmaAllocator allocator);

// --- Buffer operations ---

// Create a buffer with VMA suballocation. Replaces the vkCreateBuffer +
// vkAllocateMemory + vkBindBufferMemory triple.
VkResult vmaBridgeCreateBuffer(VmaAllocator         allocator,
                               VkDeviceSize         size,
                               VkBufferUsageFlags   usage,
                               VkSharingMode        sharingMode,
                               VmaBridgeMemoryUsage memUsage,
                               VkBuffer*            outBuffer,
                               VmaAllocation*       outAllocation);

// Create a buffer that is persistently mapped at creation time.
// Returns the mapped pointer directly — no separate map call needed.
// Used for UBOs that are written every frame.
VkResult vmaBridgeCreateBufferMapped(VmaAllocator       allocator,
                                     VkDeviceSize       size,
                                     VkBufferUsageFlags usage,
                                     VkSharingMode      sharingMode,
                                     VkBuffer*          outBuffer,
                                     VmaAllocation*     outAllocation,
                                     void**             outMappedPtr);

// Destroy a VMA-managed buffer. Replaces vkDestroyBuffer + vkFreeMemory.
void vmaBridgeDestroyBuffer(VmaAllocator  allocator,
                            VkBuffer      buffer,
                            VmaAllocation allocation);

// Map a VMA allocation for CPU access. VMA tracks the offset/size internally.
VkResult vmaBridgeMapMemory(VmaAllocator  allocator,
                            VmaAllocation allocation,
                            void**        outData);

// Unmap a previously mapped VMA allocation.
void vmaBridgeUnmapMemory(VmaAllocator  allocator,
                          VmaAllocation allocation);

// --- Image operations ---

// Create an image with VMA suballocation. Replaces vkCreateImage +
// vkAllocateMemory + vkBindImageMemory. The VkImageCreateInfo is passed
// directly since images have too many parameters for a simplified wrapper.
VkResult vmaBridgeCreateImage(VmaAllocator              allocator,
                              const VkImageCreateInfo*  imageInfo,
                              VmaBridgeMemoryUsage      memUsage,
                              VkImage*                  outImage,
                              VmaAllocation*            outAllocation);

// Destroy a VMA-managed image. Replaces vkDestroyImage + vkFreeMemory.
void vmaBridgeDestroyImage(VmaAllocator  allocator,
                           VkImage       image,
                           VmaAllocation allocation);

// --- Statistics ---

// Build a JSON string with VMA allocation statistics.
// Caller must free the returned string with vmaBridgeFreeStatsString().
char* vmaBridgeBuildStatsString(VmaAllocator allocator);

// Free a string previously returned by vmaBridgeBuildStatsString().
void vmaBridgeFreeStatsString(VmaAllocator allocator, char* statsString);

#ifdef __cplusplus
}
#endif
