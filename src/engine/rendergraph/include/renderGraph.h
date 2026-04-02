#pragma once
#include "renderGraphTypes.h"

// --- Lifecycle ---

void rgInit(RenderGraph* rg, VkDevice device, PhysicalDevice* pd,
            uint32_t swapchainWidth, uint32_t swapchainHeight,
            uint32_t swapchainImageCount);

void rgDestroy(RenderGraph* rg);

// --- Building the graph (call before rgCompile) ---

uint32_t rgAddResource(RenderGraph* rg, RGResource desc);
uint32_t rgAddExternalResource(RenderGraph* rg, RGResource desc);
uint32_t rgAddPass(RenderGraph* rg, RGPassType type);

void rgPassAddInput(RenderGraph* rg, uint32_t passIndex, RGResourceUsage usage);
void rgPassAddOutput(RenderGraph* rg, uint32_t passIndex, RGResourceUsage usage);
void rgPassSetCallback(RenderGraph* rg, uint32_t passIndex, RGPassExecuteFn fn, void* userData);

// --- Compilation ---

void rgCompileSortAndBarriers(RenderGraph* rg);
void rgCompile(RenderGraph* rg);

// --- Execution (called every frame) ---

void rgExecute(RenderGraph* rg, VkCommandBuffer cmd, uint32_t frameIndex);

// --- Debug ---

void rgDumpCompiled(const RenderGraph* rg);

// --- Swapchain resize ---

void rgOnSwapchainResize(RenderGraph* rg, uint32_t newWidth, uint32_t newHeight);
