#pragma once

#include <stdint.h>
#include <vulkan/vulkan.h>

// A11 — Bindless resource system types.
//
// A single engine-lifetime descriptor set holds two giant arrays:
//   binding 0: sampled images (textures + sampler pairs)
//   binding 1: storage buffers (transforms, particles, lights, ...)
//
// Resources register for a slot index (BindlessHandle) and shaders access
// them via push-constant-provided indices. Descriptors use
// UPDATE_AFTER_BIND + PARTIALLY_BOUND so registration is not synchronized
// against frame submission — a slot can be (re)written at any time as long
// as the slot itself isn't referenced by in-flight commands.

typedef uint32_t BindlessHandle;
#define BINDLESS_INVALID UINT32_MAX

// Binding indices inside the global descriptor set (set 0).
#define BINDLESS_SET_INDEX      0u
#define BINDLESS_BINDING_TEXTURES 0u
#define BINDLESS_BINDING_BUFFERS  1u

typedef struct BindlessSystem
{
    VkDevice              device;

    VkDescriptorSetLayout globalLayout;
    VkDescriptorPool      globalPool;
    VkDescriptorSet       globalSet;

    // Texture slot bookkeeping (set 0, binding 0).
    uint32_t              maxTextures;
    uint32_t              textureCount;
    uint32_t*             textureFreeList;
    uint32_t              textureFreeListCount;

    // Storage buffer slot bookkeeping (set 0, binding 1).
    uint32_t              maxBuffers;
    uint32_t              bufferCount;
    uint32_t*             bufferFreeList;
    uint32_t              bufferFreeListCount;
} BindlessSystem;
