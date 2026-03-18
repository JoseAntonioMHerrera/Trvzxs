// VMA Implementation Translation Unit
//
// VMA is a C++ library internally. This file compiles as C++ so VMA can use
// <cstdint>, <algorithm>, etc. The rest of the engine (pure C) never includes
// <vk_mem_alloc.h> directly — it goes through vma_bridge.h instead.
//
// The VMA_IMPLEMENTATION macro generates the implementation code (function
// bodies, static data). This must appear in exactly one translation unit.

// Tell VMA to use statically-linked Vulkan functions (vkAllocateMemory, etc.)
// rather than loading them at runtime via vkGetDeviceProcAddr. This is correct
// because our engine links Vulkan::Vulkan directly in CMake.
#define VMA_STATIC_VULKAN_FUNCTIONS  1
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0

// Generate the implementation
#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>
