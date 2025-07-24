#pragma once
#include "vertex.h"
#include "array.h"
typedef struct GEOM_PARTICLE {
    union GEOM_VERTEX_POSITION position;
    float pad;
    union GEOM_VERTEX_VELOCITY velocity;
    float pad2; // std140 layout, vec3-> vec4 for 16 bytes each
    struct GEOM_VERTEX_COLOR_RGBA color;
} GEOM_PARTICLE;

typedef struct ParticleGroup {
    uint32_t particleLen;
    uint32_t particleMaxLen;
    GEOM_PARTICLE *pParticle;
} ParticleGroup;


typedef struct ParticleGroupManager {
    ArrayUint32_t offsets;
    ParticleGroup* pParticleGroup;
    uint32_t particleGroupLen;
}ParticleGroupManager;
#define PARTICLE_VERT_SHADER "shaders/bin/particle.vert.spv\0"
#define PARTICLE_FRAG_SHADER "shaders/bin/particle.frag.spv\0"
