#pragma once
#include "vertex.h"
#include "array.h"
typedef struct ParticleInstance
{
    union GEOM_VERTEX_POSITION    position;
    float                         pad;
    union GEOM_VERTEX_VELOCITY    velocity;
    float                         pad2; // std140 layout, vec3-> vec4 for 16 bytes each
    struct GEOM_VERTEX_COLOR_RGBA color;
    uint32_t                      numberOfVertex;
} ParticleInstance;

typedef struct ParticleGroup
{
    uint32_t          particleInstancesLen;
    uint32_t          particleMaxLen;
    ParticleInstance* pParticleInstances;
} ParticleGroup;

typedef struct ParticleGroupManager
{
    uint32_t       particlesInstancesTotalLen;
    ParticleGroup* pParticleGroup;
    uint32_t       particleGroupLen;

    ParticleInstance* pParticleInstance;
    uint32_t          packedParticleInstanceLen;
    // Geometry and indices
    GEOM_VERTEX* pPackedParticleVertex;
    uint32_t     packedParticleVertexLen;
    uint32_t*    pPackedParticleIndices;
    uint32_t     packedParticleIndicesLen;

    ArrayUint32_t particleInstanceOffsets;

} ParticleGroupManager;
#define PARTICLE_VERT_SHADER "shaders/bin/particle.vert.spv\0"
#define PARTICLE_FRAG_SHADER "shaders/bin/particle.frag.spv\0"
