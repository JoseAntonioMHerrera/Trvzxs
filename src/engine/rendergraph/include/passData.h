#pragma once
#include "geometryPipelineTypes.h"
#include "deviceManagmentTypes.h"

// Per-pass userData structs — passed to RGPassExecuteFn callbacks via void* userData.
// Each pass callback casts userData to the appropriate type to access its pipeline,
// descriptor sets, and scene data.

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
