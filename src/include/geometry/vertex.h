#pragma once
#include <stdint.h>

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

struct GEOM_VERTEX {
    struct GEOM_VERTEX_POSITION point;
    struct GEOM_VERTEX_COLOR_RGBA color;
};

struct GEOM_OBJECT {
    struct GEOM_VERTEX *data;
    uint32_t dataLen;
    uint32_t *indices;
    uint32_t indicesLen;
};

typedef struct GEOM_OBJECT GEOM_OBJECT;
typedef struct GEOM_VERTEX GEOM_VERTEX;
typedef struct GEOM_VERTEX_COLOR_RGBA GEOM_VERTEX_COLOR_RGBA;
typedef struct GEOM_VERTEX_POSITION GEOM_VERTEX_POSITION;
