#pragma once
#include <stdint.h>
#include "cglm.h"
#include "numberTypes.h"
typedef union GEOM_VERTEX_POSITION
{
    struct
    {
        float x;
        float y;
        float z;
    };
    float data[3];
} GEOM_VERTEX_POSITION;

typedef union GEOM_VERTEX_VELOCITY
{
    struct
    {
        float x;
        float y;
        float z;
    };
    float data[3];
} GEOM_VERTEX_VELOCITY;
typedef struct GEOM_VERTEX_COLOR_RGBA
{
    float r;
    float g;
    float b;
    float a;
} GEOM_VERTEX_COLOR_RGBA;

typedef struct GEOM_VERTEX_TEXT_COORD
{
    float u;
    float v;
    float w;
} GEOM_VERTEX_TEXT_COORD;

typedef struct GEOM_VERTEX_NORMAL
{
    float x;
    float y;
    float z;
} GEOM_VERTEX_NORMAL;

typedef struct TrzVertex
{
    vec4 point;
    vec4 color;
    vec4 textCoords;
    vec4 normal; // Added normal
    vec4 bary;
} TrzVertex;

typedef struct GEOM_VERTEX
{
    union GEOM_VERTEX_POSITION    point;
    struct GEOM_VERTEX_COLOR_RGBA color;
    struct GEOM_VERTEX_TEXT_COORD textCoords;
    struct GEOM_VERTEX_NORMAL     normal; // Added normal
    struct
    {
        float x, y, z;
    } bary;
} GEOM_VERTEX;

// Lean vertex for model rendering — drops dead color and unused bary (64B → 32B)
typedef struct MODEL_VERTEX
{
    union GEOM_VERTEX_POSITION point;     // 12 bytes (vec3)
    struct { float u, v; }     texCoord;  // 8 bytes (vec2)
    struct GEOM_VERTEX_NORMAL  normal;    // 12 bytes (vec3)
} MODEL_VERTEX;
typedef struct GEOM_VERTEX_G_BUFFER
{
    union GEOM_VERTEX_POSITION    geometry;
    union GEOM_VERTEX_POSITION    normals;
    struct GEOM_VERTEX_COLOR_RGBA albedo;
} GEOM_VERTEX_G_BUFFER;

typedef struct GEOM_OBJECT
{

    struct GEOM_VERTEX* data;
    uint32_t            dataLen;

    TRVZXS_SIZE* indices;
    uint32_t     indicesLen;

    TRVZXS_SIZE vertexOffset;

} GEOM_OBJECT;
