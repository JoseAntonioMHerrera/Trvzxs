#include <stdlib.h>
#include <math.h>
#include "vertex.h"
// Helper function to create a vertex
GEOM_VERTEX* create_vertex(float x, float y, float z, float r, float g, float b, float u, float v) {
    GEOM_VERTEX *vertex = malloc(sizeof(GEOM_VERTEX));
    vertex->point.x = x;
    vertex->point.y = y;
    vertex->point.z = z;
    vertex->color.a = 1.0f;
    vertex->color.g = 1.0f;
    vertex->color.r = 1.0f;
    vertex->color.b = 1.0f;
    vertex->textCoords.u = u;
    vertex->textCoords.v = v;
    vertex->textCoords.w = 0.0f;

    return vertex;
}
/*
GEOM_OBJECT* create_grid(int numPoints, float padding) {
    GEOM_OBJECT* grid = malloc(sizeof(GEOM_OBJECT));
    int numVertices = numPoints * numPoints;
    int numIndices = (numPoints - 1) * (numPoints - 1) * 6; // 2 triangles per quad

    grid->dataLen = numVertices;
    grid->data = malloc(sizeof(GEOM_VERTEX) * numVertices);
    grid->indicesLen = numIndices;
    grid->indices = malloc(sizeof(TRVZXS_SIZE) * numIndices);

    // Create vertices with texture coordinates
    for (int y = 0; y < numPoints; ++y) {
        for (int x = 0; x < numPoints; ++x) {
            float xpos = x * padding;
            float ypos = y * padding;

            // Normalize texture coordinates to cover [0, 1] range
            float u = (float)x / (numPoints - 1); // Horizontal coordinate in texture space
            float v = (float)y / (numPoints - 1); // Vertical coordinate in texture space

            // Create vertex with position and texture coordinates
            grid->data[y * numPoints + x] = create_vertex(xpos, ypos, 0.0f, 1.0f, 1.0f, 1.0f, u, v);
        }
    }

    // Create indices and assign texture coordinates for each quad
    int index = 0;
    for (int y = 0; y < numPoints - 1; ++y) {
        for (int x = 0; x < numPoints - 1; ++x) {
            int topLeft = y * numPoints + x;
            int topRight = topLeft + 1;
            int bottomLeft = (y + 1) * numPoints + x;
            int bottomRight = bottomLeft + 1;

            // Define texture coordinates for each vertex
            float u0 = (float)x / (numPoints - 1);
            float v0 = (float)y / (numPoints - 1);
            float u1 = (float)(x + 1) / (numPoints - 1);
            float v1 = (float)(y + 1) / (numPoints - 1);

            // Define indices for two triangles per quad (counter-clockwise winding order)
            grid->indices[index++] = topLeft;
            grid->indices[index++] = bottomLeft;
            grid->indices[index++] = topRight;

            grid->indices[index++] = topRight;
            grid->indices[index++] = bottomLeft;
            grid->indices[index++] = bottomRight;

            // Ensure each vertex has correct texture coordinates
            grid->data[topLeft].textCoords.u = u0;
            grid->data[topLeft].textCoords.v = v0;

            grid->data[topRight].textCoords.u = u1;
            grid->data[topRight].textCoords.v = v0;

            grid->data[bottomLeft].textCoords.u = u0;
            grid->data[bottomLeft].textCoords.v = v1;

            grid->data[bottomRight].textCoords.u = u1;
            grid->data[bottomRight].textCoords.v = v1;
        }
    }

    return grid;
}*/


// Cube
GEOM_OBJECT* create_cube(float size, uint32_t offset) {
    GEOM_OBJECT* cube = malloc(sizeof(GEOM_OBJECT));
    cube->dataLen = 24; // 6 faces * 4 vertices each
    cube->data = malloc(sizeof(GEOM_VERTEX) * cube->dataLen);
    cube->indicesLen = 36; // 6 faces * 2 triangles * 3 vertices each
    cube->indices = malloc(sizeof(uint32_t) * cube->indicesLen);

    float s = size / 2;

    // Front face
    cube->data[0] = *create_vertex(-s + offset, -s + offset,  s + offset, 1, 0, 0, 0, 0);
    cube->data[1] = *create_vertex( s + offset, -s + offset,  s + offset, 1, 0, 0, 1, 0);
    cube->data[2] = *create_vertex( s + offset,  s + offset,  s + offset, 1, 0, 0, 1, 1);
    cube->data[3] = *create_vertex(-s + offset,  s + offset,  s + offset, 1, 0, 0, 0, 1);

    // Back face
    cube->data[4] = *create_vertex(-s + offset, -s + offset, -s + offset, 0, 1, 0, 0, 0);
    cube->data[5] = *create_vertex(-s + offset,  s + offset, -s + offset, 0, 1, 0, 1, 0);
    cube->data[6] = *create_vertex( s + offset,  s + offset, -s + offset, 0, 1, 0, 1, 1);
    cube->data[7] = *create_vertex( s + offset, -s + offset, -s + offset, 0, 1, 0, 0, 1);

    // Top face
    cube->data[8]  = *create_vertex(-s + offset,  s + offset, -s + offset, 0, 0, 1, 0, 0);
    cube->data[9]  = *create_vertex(-s + offset,  s + offset,  s + offset, 0, 0, 1, 1, 0);
    cube->data[10] = *create_vertex( s + offset,  s + offset,  s + offset, 0, 0, 1, 1, 1);
    cube->data[11] = *create_vertex( s + offset,  s + offset, -s + offset, 0, 0, 1, 0, 1);

    // Bottom face
    cube->data[12] = *create_vertex(-s + offset, -s + offset, -s + offset, 1, 1, 0, 0, 0);
    cube->data[13] = *create_vertex( s + offset, -s + offset, -s + offset, 1, 1, 0, 1, 0);
    cube->data[14] = *create_vertex( s + offset, -s + offset,  s + offset, 1, 1, 0, 1, 1);
    cube->data[15] = *create_vertex(-s + offset, -s + offset,  s + offset, 1, 1, 0, 0, 1);

    // Right face
    cube->data[16] = *create_vertex( s + offset, -s + offset, -s + offset, 1, 0, 1, 0, 0);
    cube->data[17] = *create_vertex( s + offset,  s + offset, -s + offset, 1, 0, 1, 1, 0);
    cube->data[18] = *create_vertex( s + offset,  s + offset,  s + offset, 1, 0, 1, 1, 1);
    cube->data[19] = *create_vertex( s + offset, -s + offset,  s + offset, 1, 0, 1, 0, 1);

    // Left face
    cube->data[20] = *create_vertex(-s + offset, -s + offset, -s + offset, 0, 1, 1, 0, 0);
    cube->data[21] = *create_vertex(-s + offset, -s + offset,  s + offset, 0, 1, 1, 1, 0);
    cube->data[22] = *create_vertex(-s + offset,  s + offset,  s + offset, 0, 1, 1, 1, 1);
    cube->data[23] = *create_vertex(-s + offset,  s + offset, -s + offset, 0, 1, 1, 0, 1);

    // Indices for all 6 faces
    TRVZXS_SIZE indices[] = {
        0, 1, 2, 2, 3, 0,       // Front face
        4, 5, 6, 6, 7, 4,       // Back face
        8, 9, 10, 10, 11, 8,    // Top face
        12, 13, 14, 14, 15, 12, // Bottom face
        16, 17, 18, 18, 19, 16, // Right face
        20, 21, 22, 22, 23, 20  // Left face
    };

    for (int i = 0; i < cube->indicesLen; i++) {
        cube->indices[i] = indices[i];
    }

    return cube;
}
/*
// Pyramid
GEOM_OBJECT* create_pyramid(float base_size, float height) {
    GEOM_OBJECT* pyramid = malloc(sizeof(GEOM_OBJECT));
    pyramid->dataLen = 5; // 4 vertices for base + 1 for top
    pyramid->data = malloc(sizeof(GEOM_VERTEX) * pyramid->dataLen);
    pyramid->indicesLen = 18; // 6 triangles * 3 vertices each
    pyramid->indices = malloc(sizeof(TRVZXS_SIZE) * pyramid->indicesLen);

    float s = base_size / 2;
    float h = height / 2;

    // Base vertices
    pyramid->data[0] = create_vertex(-s, -h, -s, 1, 0, 0, 0, 0);
    pyramid->data[1] = create_vertex( s, -h, -s, 0, 1, 0, 1, 0);
    pyramid->data[2] = create_vertex( s, -h,  s, 0, 0, 1, 1, 1);
    pyramid->data[3] = create_vertex(-s, -h,  s, 1, 1, 0, 0, 1);

    // Top vertex
    pyramid->data[4] = create_vertex(0, h, 0, 1, 1, 1, 0.5, 0.5);

    TRVZXS_SIZE indices[] = {
        0, 1, 2,  0, 2, 3,  // Base
        0, 4, 1,            // Side 1
        1, 4, 2,            // Side 2
        2, 4, 3,            // Side 3
        3, 4, 0             // Side 4
    };

    for (int i = 0; i < pyramid->indicesLen; i++) {
        pyramid->indices[i] = indices[i];
    }

    return pyramid;
}

// Rhomboid
GEOM_OBJECT* create_rhomboid(float width, float height, float depth, float skew) {
    GEOM_OBJECT* rhomboid = malloc(sizeof(GEOM_OBJECT));
    rhomboid->dataLen = 8; // 8 vertices
    rhomboid->data = malloc(sizeof(GEOM_VERTEX) * rhomboid->dataLen);
    rhomboid->indicesLen = 36; // 6 faces * 2 triangles * 3 vertices each
    rhomboid->indices = malloc(sizeof(TRVZXS_SIZE) * rhomboid->indicesLen);

    float w = width / 2, h = height / 2, d = depth / 2;

    // Vertices
    rhomboid->data[0] = create_vertex(-w, -h, -d, 1, 0, 0, 0, 0);
    rhomboid->data[1] = create_vertex( w, -h, -d, 0, 1, 0, 1, 0);
    rhomboid->data[2] = create_vertex( w+skew,  h, -d, 0, 0, 1, 1, 1);
    rhomboid->data[3] = create_vertex(-w+skew,  h, -d, 1, 1, 0, 0, 1);
    rhomboid->data[4] = create_vertex(-w, -h,  d, 1, 0, 1, 0, 0);
    rhomboid->data[5] = create_vertex( w, -h,  d, 0, 1, 1, 1, 0);
    rhomboid->data[6] = create_vertex( w+skew,  h,  d, 1, 1, 1, 1, 1);
    rhomboid->data[7] = create_vertex(-w+skew,  h,  d, 0.5, 0.5, 0.5, 0, 1);

    TRVZXS_SIZE indices[] = {
        0, 1, 2, 2, 3, 0,  // Front face
        4, 7, 6, 6, 5, 4,  // Back face
        0, 3, 7, 7, 4, 0,  // Left face
        1, 5, 6, 6, 2, 1,  // Right face
        3, 2, 6, 6, 7, 3,  // Top face
        0, 4, 5, 5, 1, 0   // Bottom face
    };

    for (int i = 0; i < rhomboid->indicesLen; i++) {
        rhomboid->indices[i] = indices[i];
    }

    return rhomboid;
}*/
