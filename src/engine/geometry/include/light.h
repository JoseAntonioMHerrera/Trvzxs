#pragma once
#include "vertex.h"
#include "model.h"
#include "camera.h"

// Light-instance alive flag values (same semantics as model instances)
#define LIGHT_INSTANCE_ALIVE     1
#define LIGHT_INSTANCE_DISABLED  0
#define LIGHT_INSTANCE_DESTROYED 2

// Light-type alive flag values (same semantics as model types)
#define LIGHT_ALIVE     1
#define LIGHT_DISABLED  2
#define LIGHT_DESTROYED 0

typedef struct LightProperties
{
    uint8_t  transparency;
    uint32_t intensity;
    uint32_t fallOf;
} LightProperties;
typedef struct LightVolumeInstance
{
    GEOM_VERTEX_COLOR_RGBA color;
    vec3                   position; // TODO empaquetar informacion en vec4 estructuras para gpu
    float                  padding;
    float                  scale;
    uint32_t               alive;      // LIGHT_INSTANCE_ALIVE/DISABLED/DESTROYED
    float                  padding2[2]; // std140 requires 48-byte struct size (vec4 alignment)
} LightVolumeInstance;
typedef struct LightTransformations
{
    struct MODEL_VIEW_PROJECTION mvp;
    mat4                         mInverseVP;
    struct Camera3D              camera;
    struct LightVolumeInstance   data;
} LightTransformations;

typedef struct LightVolume
{
    LightProperties      properties;
    Model3D              model;
    LightVolumeInstance* pLightVolumeInstance;
    uint32_t             lightVolumeInstanceCount; // total allocated (capacity)
    uint32_t             liveInstanceCount;        // computed per frame by UBO fill
    uint32_t             alive;                    // LIGHT_ALIVE/DISABLED/DESTROYED
} LightVolume;

typedef struct LightManager
{
    LightVolume* pLight;
    uint32_t     lightLen;

    uint32_t lightInstanceCapacity; // total instance slots across all lights (immutable after init)
    uint32_t lightInstanceCount;    // live instance count (updated per frame by UBO fill)

    GEOM_VERTEX* pPackedLight;
    uint32_t     packedLightLen;

    // Packed instance data
    //  ****-****
    //
    ArrayUint32_t packedLightIndex;
    ArrayUint32_t vertexOffset;
    ArrayUint32_t indiceInstanceOffset;
    ArrayUint32_t instanceOffset;
} LightManager;
#define VOLUMETRIC_LIGHT_VERT_SHADER "shaders/bin/lightVolume.vert.spv\0"
#define VOLUMETRIC_LIGHT_FRAG_SHADER "shaders/bin/lightVolume.frag.spv\0"

/*

    Model3D* pModel;
    uint32_t modelLen;
    Model3DInstance* pModelInstance;
    uint32_t modelInstanceLen;

    GEOM_VERTEX* pPackedModelVertex;
    uint32_t packedModelVertexLen;

    ArrayUint32_t packedModelIndex;

    ArrayUint32_t vertexOffset;
    ArrayUint32_t indiceInstanceOffset;
    ArrayUint32_t instanceOffset;


*/
