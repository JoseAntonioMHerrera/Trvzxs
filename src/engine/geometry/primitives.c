#include <stdlib.h>
#include <math.h>
#include "vertex.h"
#include "string.h"
#include "primitives.h"

void printParticle(const GEOM_PARTICLE* particle){
    printf("Particle Data:\n");
    printf("Position: (x: %.4f, y: %.4f, z: %.4f)\n",
        particle->position.x,
        particle->position.y,
        particle->position.z);

    printf("Velocity: (x: %.4f, y: %.4f, z: %.4f)\n",
        particle->velocity.x,
        particle->velocity.y,
        particle->velocity.z);

    printf("Color: (r: %.4f, g: %.4f, b: %.4f, a: %.4f)\n",
        particle->color.r,
        particle->color.g,
        particle->color.b,
        particle->color.a);
    printf("------------------------\n");
}
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

void print_cube(const GEOM_OBJECT* cube) {
    printf("Cube Information:\n");
    printf("Number of vertices: %u\n", cube->dataLen);
    printf("Number of indices: %u\n", cube->indicesLen);
    printf("Vertex offset: %u\n", cube->vertexOffset);

    printf("\nVertices:\n");
    for (uint32_t i = 0; i < cube->dataLen; i++) {
        printf("Vertex %u: (%.2f, %.2f, %.2f) UV: (%.2f, %.2f) Normal: (%.2f, %.2f, %.2f)\n",
               i,
               cube->data[i].point.x, cube->data[i].point.y, cube->data[i].point.z,
               cube->data[i].textCoords.u, cube->data[i].textCoords.v);
    };

    printf("\nIndices:\n");
    for (uint32_t i = 0; i < cube->indicesLen; i += 3) {
        printf("Triangle %u: %u, %u, %u\n", i / 3, cube->indices[i], cube->indices[i+1], cube->indices[i+2]);
    }
}
// Cube

GEOM_OBJECT* create_quad(const GEOM_VERTEX_POSITION* const globalCoords, uint32_t size) {
    GEOM_OBJECT* quad = (GEOM_OBJECT*)malloc(sizeof(GEOM_OBJECT));
    quad->dataLen = 4; // 8 vertices for a cube
    quad->data = (GEOM_VERTEX*)malloc(sizeof(GEOM_VERTEX) * quad->dataLen);
    quad->indicesLen = 6; // 6 faces * 2 triangles * 3 vertices each
    quad->indices = (uint32_t*)malloc(sizeof(uint32_t) * quad->indicesLen);
    quad->vertexOffset = 4;

    float s = size / 2.0f;
    float cx = globalCoords->x;
    float cy = globalCoords->y;
    float cz = globalCoords->z;

    // Define vertices
GEOM_VERTEX vertices[4] = {
    {cx - s, cy - s, cz - s, 0, 0, 0, 0, 0, 0}, // 0: Left  Bottom Back
    {cx + s, cy - s, cz - s, 1, 0, 0, 1, 1, 0}, // 1: Right Bottom Back
    {cx + s, cy + s, cz - s, 1, 1, 0, 1, 1, 1}, // 2: Right Top    Back
    {cx - s, cy + s, cz - s, 0, 1, 0, 0, 0, 1}, // 3: Left  Top    Back
};

    // Copy vertices to cube data
    memcpy(quad->data, vertices, sizeof(vertices));

    // Define indices for all 6 faces
    uint32_t indices[] = {
        0, 3, 2, 0, 2, 1, // Back face
    };

    // Copy indices to cube
    memcpy(quad->indices, indices, sizeof(indices));
    //print_cube(cube);
    return quad;
}

GEOM_OBJECT* create_cube(const GEOM_VERTEX_POSITION* const globalCoords, uint32_t size) {
    GEOM_OBJECT* cube = (GEOM_OBJECT*)malloc(sizeof(GEOM_OBJECT));
    cube->dataLen = 8; // 8 vertices for a cube
    cube->data = (GEOM_VERTEX*)malloc(sizeof(GEOM_VERTEX) * cube->dataLen);
    cube->indicesLen = 36; // 6 faces * 2 triangles * 3 vertices each
    cube->indices = (uint32_t*)malloc(sizeof(uint32_t) * cube->indicesLen);
    cube->vertexOffset = 8;

    float s = size / 2.0f;
    float cx = globalCoords->x;
    float cy = globalCoords->y;
    float cz = globalCoords->z;

    // Define vertices
GEOM_VERTEX vertices[8] = {
    {cx - s, cy - s, cz - s, 0, 0, 0, 0, 0, 0}, // 0: Left  Bottom Back
    {cx + s, cy - s, cz - s, 1, 0, 0, 1, 1, 0}, // 1: Right Bottom Back
    {cx + s, cy + s, cz - s, 1, 1, 0, 1, 1, 1}, // 2: Right Top    Back
    {cx - s, cy + s, cz - s, 0, 1, 0, 0, 0, 1}, // 3: Left  Top    Back
    {cx - s, cy - s, cz + s, 0, 0, 1, 0, 0, 0}, // 4: Left  Bottom Front
    {cx + s, cy - s, cz + s, 1, 0, 1, 1, 1, 0}, // 5: Right Bottom Front
    {cx + s, cy + s, cz + s, 1, 1, 1, 1, 1, 1}, // 6: Right Top    Front
    {cx - s, cy + s, cz + s, 0, 1, 1, 0, 0, 1}  // 7: Left  Top    Front
};

    // Copy vertices to cube data
    memcpy(cube->data, vertices, sizeof(vertices));

    // Define indices for all 6 faces
    uint32_t indices[] = {
        4, 5, 6, 6, 7, 4, // Front face
        0, 3, 2, 2, 1, 0, // Back face
        3, 7, 6, 6, 2, 3, // Top face
        0, 1, 5, 5, 4, 0, // Bottom face
        1, 2, 6, 6, 5, 1, // Right face
        0, 4, 7, 7, 3, 0  // Left face
    };

    // Copy indices to cube
    memcpy(cube->indices, indices, sizeof(indices));
    //print_cube(cube);
    return cube;
}
