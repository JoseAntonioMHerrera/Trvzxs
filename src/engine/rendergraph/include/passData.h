#pragma once

typedef struct GraphicPipelineToSurface GraphicPipelineToSurface;
typedef struct DisplayManager           DisplayManager;
typedef struct PhysicalDevice           PhysicalDevice;
typedef struct LogicalDevice            LogicalDevice;

// Per-pass userData structs — passed to RGPassExecuteFn callbacks via void* userData.
// Each pass callback casts userData to the appropriate type to access its pipeline,
// descriptor sets, and scene data.

typedef struct DepthPrepassData
{
    GraphicPipelineToSurface* depthPipeline; // depth-only pipeline (vertex shader, no fragment)
    GraphicPipelineToSurface* modelPipeline; // model pipeline (shares vertex/index buffers + descriptors)
    DisplayManager*           dm;
} DepthPrepassData;

typedef struct ModelPassData
{
    GraphicPipelineToSurface* pipeline;
    DisplayManager*           dm;
} ModelPassData;

typedef struct LightPassData
{
    GraphicPipelineToSurface* pipeline;
    DisplayManager*           dm;
} LightPassData;

typedef struct ParticlePassData
{
    GraphicPipelineToSurface* pipeline;
    DisplayManager*           dm;
} ParticlePassData;

typedef struct ComputePassData
{
    GraphicPipelineToSurface* pipeline;
    DisplayManager*           dm;
    PhysicalDevice*           pd;
    LogicalDevice*            ld;
    float*                    deltaTime;
} ComputePassData;
