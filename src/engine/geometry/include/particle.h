#pragma once
#include "vertex.h"

typedef struct GEOM_PARTICLE {
    union GEOM_VERTEX_POSITION position;
    float pad;
    union GEOM_VERTEX_VELOCITY velocity;
    float pad2; // std140 layout, vec3-> vec4 for 16 bytes each
    struct GEOM_VERTEX_COLOR_RGBA color;
} GEOM_PARTICLE;

#define PARTICLE_VERT_SHADER "shaders/bin/particle.vert.spv\0"
#define PARTICLE_FRAG_SHADER "shaders/bin/particle.frag.spv\0"
//const uint32_t PARTICLE_COUNT = 10000;
