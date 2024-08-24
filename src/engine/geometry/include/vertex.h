#pragma once
#include <stdint.h>
#include "cglm.h"
#include "numberTypes.h"
typedef struct GEOM_VERTEX_POSITION {
    float x;
    float y;
    float z;
} GEOM_VERTEX_POSITION;

typedef struct GEOM_VERTEX_COLOR_RGBA {
    float r;
    float b;
    float g;
    float a;
} GEOM_VERTEX_COLOR_RGBA;

typedef struct GEOM_VERTEX_TEXT_COORD {
    float u;
    float v;
    float w;
} GEOM_VERTEX_TEXT_COORD;

typedef struct GEOM_VERTEX {
    struct GEOM_VERTEX_POSITION point;
    struct GEOM_VERTEX_COLOR_RGBA color;
    GEOM_VERTEX_TEXT_COORD textCoords;
} GEOM_VERTEX;

typedef struct GEOM_OBJECT {

    struct GEOM_VERTEX *data;
    uint32_t dataLen;

    TRVZXS_SIZE *indices;
    uint32_t indicesLen;

    TRVZXS_SIZE vertexOffset;

} GEOM_OBJECT;
