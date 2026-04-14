#pragma once

#include "bindlessTypes.h"

// Lifecycle. Allocates the global layout, pool, single set, and free lists.
// maxTextures / maxBuffers are hard caps checked against device limits.
void bindlessInit(BindlessSystem* bs, VkDevice device,
                  VkPhysicalDevice gpu,
                  uint32_t maxTextures, uint32_t maxBuffers);
void bindlessDestroy(BindlessSystem* bs);

// Registration. Returns a slot index usable from shaders as push-constant
// payload. Thread-unsafe; callers must serialize across submission threads.
// Textures: combined image + sampler pair. Expected layout:
// VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL at shader read time.
BindlessHandle bindlessRegisterTexture(BindlessSystem* bs,
                                       VkImageView view, VkSampler sampler);
void           bindlessUnregisterTexture(BindlessSystem* bs, BindlessHandle h);

// Storage buffers. Whole-buffer binding; shader accesses range via index.
BindlessHandle bindlessRegisterBuffer(BindlessSystem* bs,
                                      VkBuffer buffer, VkDeviceSize size);
void           bindlessUnregisterBuffer(BindlessSystem* bs, BindlessHandle h);

// Command recording. Binds the global set at set index 0 for the given
// pipeline layout + bind point. Call once per pass before issuing draws.
void bindlessBindGlobal(BindlessSystem* bs, VkCommandBuffer cmd,
                        VkPipelineLayout layout,
                        VkPipelineBindPoint bindPoint);

// Re-writes bindings 1 (G-buffer) and 2 (normals) of every per-frame
// descriptor set used by the light pass. Call on swapchain recreate after
// the G-buffer / normals images have been rebuilt; the existing writes
// point at now-destroyed image views.
void bindlessWriteLightPassGBufferSamplers(VkDevice device,
                                           uint32_t maxFrames,
                                           VkDescriptorSet* lightPerPassSets,
                                           VkImageView gBufferView,
                                           VkImageView normalsView,
                                           VkSampler sampler);
