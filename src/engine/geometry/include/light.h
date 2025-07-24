#pragma once
#include "vertex.h"
#include "model.h"

typedef struct LightProperties {
    uint8_t transparency;
    uint32_t intensity;
    uint32_t fallOf;
} LightProperties;

typedef struct LightVolume {
    LightProperties properties;
    Model3D model;
} LightVolume;

typedef struct LightVolumeInstance {
    mat4 m;
    GEOM_VERTEX_COLOR_RGBA color;
} LightVolumeInstance;
typedef struct LightManager {
    LightVolume* pLight;
    uint32_t lightLen;

    LightVolumeInstance* pLightInstance;
    uint32_t lightInstanceLen;

    GEOM_VERTEX* pPackedLight;
    uint32_t packedLightLen;

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
