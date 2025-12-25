#pragma once
#include "vertex.h"
#include "array.h"
typedef struct ParticleInstance
{
    vec4 position;
    vec4 velocity;
    vec4 color;
} ParticleInstance;

typedef struct ParticleGroup
{
    uint32_t          particleInstancesLen;
    uint32_t          particleMaxLen;
    ParticleInstance* pParticleInstances;
} ParticleGroup;

typedef struct ParticleGroupManager
{
    ParticleGroup* pParticleGroup;
    uint32_t       particleGroupLen;

    ParticleInstance* pParticleInstance;
    uint32_t          packedParticleInstanceLen;
    // Geometry and indices
    TrzVertex* pPackedParticleVertex;
    uint32_t   packedParticleVertexLen;
    uint32_t*  pPackedParticleIndices;
    uint32_t   packedParticleIndicesLen;

    ArrayUint32_t particleInstanceOffsets;

} ParticleGroupManager;
#define PARTICLE_VERT_SHADER "shaders/bin/particle.vert.spv\0"
#define PARTICLE_FRAG_SHADER "shaders/bin/particle.frag.spv\0"
