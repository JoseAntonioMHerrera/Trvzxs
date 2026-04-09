#pragma once
#include "renderGraphTypes.h"

// Pass execution callbacks for the render graph.
// Each matches the RGPassExecuteFn signature: void(VkCommandBuffer, const RGPass*, void*)
// userData is cast to the corresponding PassData struct from passData.h.

void executeDepthPrepass(VkCommandBuffer cmd, const RGPass* pass, void* userData);
void executeModelPass(VkCommandBuffer cmd, const RGPass* pass, void* userData);
void executeLightPass(VkCommandBuffer cmd, const RGPass* pass, void* userData);
void executeParticlePass(VkCommandBuffer cmd, const RGPass* pass, void* userData);
void executeParticleComputePass(VkCommandBuffer cmd, const RGPass* pass, void* userData);
