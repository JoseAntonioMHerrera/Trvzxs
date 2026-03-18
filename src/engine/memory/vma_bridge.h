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

#ifdef __cplusplus
extern "C" {
#endif

// Create a VMA allocator tied to the given Vulkan device/instance.
// Must be called after the logical device and queues are initialized.
VkResult vmaBridgeCreateAllocator(VkInstance       instance,
                                  VkPhysicalDevice physicalDevice,
                                  VkDevice         device,
                                  VmaAllocator*    outAllocator);

// Destroy a VMA allocator. Call before vkDestroyDevice.
void vmaBridgeDestroyAllocator(VmaAllocator allocator);

#ifdef __cplusplus
}
#endif
