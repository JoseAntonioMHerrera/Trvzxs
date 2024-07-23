#pragma once
#include <stdint.h>
#include "cglm.h"
#include "numberTypes.h"
struct GEOM_VERTEX_POSITION {
    float x;
    float y;
    float z;
};

struct GEOM_VERTEX_COLOR_RGBA {
    float r;
    float b;
    float g;
    float a;
};

typedef struct GEOM_VERTEX_TEXT_COORD {
    float u;
    float v;
    float w;
} GEOM_VERTEX_TEXT_COORD;

struct GEOM_VERTEX {
    struct GEOM_VERTEX_POSITION point;
    struct GEOM_VERTEX_COLOR_RGBA color;
    GEOM_VERTEX_TEXT_COORD textCoords;
};

struct GEOM_OBJECT {

    struct GEOM_VERTEX *data;
    vec3* normal;
    vec2* textCoord;

    uint32_t dataLen;

    TRVZXS_SIZE *indices;
    uint32_t indicesLen;

};

typedef struct GEOM_OBJECT GEOM_OBJECT;
typedef struct GEOM_VERTEX GEOM_VERTEX;
typedef struct GEOM_VERTEX_COLOR_RGBA GEOM_VERTEX_COLOR_RGBA;
typedef struct GEOM_VERTEX_POSITION GEOM_VERTEX_POSITION;
