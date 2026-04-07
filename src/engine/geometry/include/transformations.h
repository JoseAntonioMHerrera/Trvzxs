#pragma once
#include "mat4.h"
#include "camera.h"
#include "vertex.h"

typedef struct MODEL_VIEW_PROJECTION
{
    mat4 model;
    mat4 view;
    mat4 proj;
} MODEL_VIEW_PROJECTION;

// --- Per-frame globals: written once per frame, shared across all pipelines (binding 9) ---

typedef struct FrameGlobals
{
    mat4     view;
    mat4     proj;
    mat4     viewProj;    // precomputed: proj * view
    mat4     invViewProj; // precomputed: inverse(viewProj)
    Camera3D camera;      // eye, center, up, screenSize (std140-aligned)
} FrameGlobals;

// --- Slim per-instance UBO structs (future phases will migrate to these) ---

typedef struct ModelInstanceUBO
{
    mat4 model; // world matrix from TransformNode
} ModelInstanceUBO;

typedef struct LightInstanceUBO
{
    mat4                   model; // world matrix from TransformNode
    GEOM_VERTEX_COLOR_RGBA color;
    vec3                   position;
    float                  scale; // uniform scale for attenuation/radius
} LightInstanceUBO;

typedef struct UboDeltaTime
{
    float deltaTime;
} UboDeltaTime;
