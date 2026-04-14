#pragma once

#include <vulkan/vulkan.h>

// A11 Phase 2 — per-pass descriptor set layouts (set index 1).
//
// Set 0 is the global bindless table (textures + storage buffers) from
// bindless.h. Set 1 holds the small residue each pass still needs — camera
// UBOs, subpass input attachments — anything that can't or shouldn't live in
// the global bindless arrays.
//
// These layouts are created once at init time alongside the bindless system
// and consumed by pipeline layout creation (Phase 4). Descriptor sets
// against these layouts are allocated per-frame (Phase 5).
//
// Particle-render intentionally has no set 1: its only inputs are bindless
// SSBOs (particle buffer) and vertex input; no per-pass resources exist.

typedef struct PerPassLayouts
{
    VkDevice              device;
    VkDescriptorSetLayout modelPass;          // binding 0: frame globals UBO
    VkDescriptorSetLayout depthPrepassPass;   // binding 0: frame globals UBO
    VkDescriptorSetLayout lightPass;          // b0 UBO + b1,b2 input attachments
    VkDescriptorSetLayout particleComputePass;// binding 0: delta time UBO
} PerPassLayouts;

void perPassLayoutsCreate(PerPassLayouts* pp, VkDevice device);
void perPassLayoutsDestroy(PerPassLayouts* pp);
