#include <stdlib.h>
#include <math.h>
#include "vertex.h"
#include "string.h"
#include "primitives.h"
#include "model.h"

void printParticle(const ParticleInstance* particle) {}
// Helper function to create a vertex
GEOM_VERTEX* create_vertex(float x, float y, float z, float r, float g, float b, float u, float v)
{
    GEOM_VERTEX* vertex  = malloc(sizeof(GEOM_VERTEX));
    vertex->point.x      = x;
    vertex->point.y      = y;
    vertex->point.z      = z;
    vertex->color.a      = 1.0f;
    vertex->color.g      = 1.0f;
    vertex->color.r      = 1.0f;
    vertex->color.b      = 1.0f;
    vertex->textCoords.u = u;
    vertex->textCoords.v = v;
    vertex->textCoords.w = 0.0f;

    return vertex;
}

Model3D* create_plane(const GEOM_VERTEX_POSITION* const globalCoords, uint32_t size)
{
    const float TILE_SIZE = 3.0f;

    // Calculate number of tiles in each dimension
    int num_tiles = (int) (size / TILE_SIZE);

    // Calculate number of vertices and indices needed
    int num_vertices_per_side = num_tiles + 1;
    int total_vertices        = num_vertices_per_side * num_vertices_per_side;
    int total_indices         = num_tiles * num_tiles * 6; // 2 triangles per tile, 3 vertices per triangle

    // Create and initialize the Model3D
    Model3D* model = (Model3D*) malloc(sizeof(Model3D));

    // Initialize meshManager with a single mesh
    model->meshManager.pMesh                = (Mesh3D*) malloc(sizeof(Mesh3D));
    model->meshManager.meshesLen            = 1;
    model->meshManager.totalMeshVerticesLen = total_vertices;

    // Initialize the single mesh
    model->meshManager.pMesh->pVertice    = (GEOM_VERTEX*) malloc(sizeof(GEOM_VERTEX) * total_vertices);
    model->meshManager.pMesh->verticesLen = total_vertices;

    // Initialize indices array
    model->indices.items    = (uint32_t*) malloc(sizeof(uint32_t) * total_indices);
    model->indices.itemsLen = total_indices;

    // Set defaults
    model->vertexOffset  = 0;
    model->indiceOffset  = 0;
    model->instanceCount = 1;
    model->pChild        = NULL;
    model->childLen      = 0;

    // Starting coordinates (top-left corner of plane)
    float start_x = globalCoords->x - size / 2.0f;
    float start_y = globalCoords->y - size / 2.0f;
    float z       = globalCoords->z;

    // Generate vertices
    for (int row = 0; row < num_vertices_per_side; row++)
    {
        for (int col = 0; col < num_vertices_per_side; col++)
        {
            int idx = row * num_vertices_per_side + col;

            // Position
            model->meshManager.pMesh->pVertice[idx].point.x = start_x + col * TILE_SIZE;
            model->meshManager.pMesh->pVertice[idx].point.y = start_y + row * TILE_SIZE;
            model->meshManager.pMesh->pVertice[idx].point.z = z;

            // Normal (pointing up Z axis)
            model->meshManager.pMesh->pVertice[idx].normal.x = 0.0f;
            model->meshManager.pMesh->pVertice[idx].normal.y = 0.0f;
            model->meshManager.pMesh->pVertice[idx].normal.z = 1.0f;

            // Texture coordinates (normalized from 0 to 1)
            model->meshManager.pMesh->pVertice[idx].textCoords.u = (float) col / num_tiles;
            model->meshManager.pMesh->pVertice[idx].textCoords.v = (float) row / num_tiles;
            model->meshManager.pMesh->pVertice[idx].textCoords.w = 0.0f;

            // Default color (white)
            model->meshManager.pMesh->pVertice[idx].color.r = 1.0f;
            model->meshManager.pMesh->pVertice[idx].color.g = 1.0f;
            model->meshManager.pMesh->pVertice[idx].color.b = 1.0f;
            model->meshManager.pMesh->pVertice[idx].color.a = 1.0f;
        }
    }

    // Generate indices for triangles
    int index = 0;
    for (int row = 0; row < num_tiles; row++)
    {
        for (int col = 0; col < num_tiles; col++)
        {
            int vertex_idx = row * num_vertices_per_side + col;

            // First triangle of tile
            model->indices.items[index++] = vertex_idx;                         // Bottom left
            model->indices.items[index++] = vertex_idx + num_vertices_per_side; // Top left
            model->indices.items[index++] = vertex_idx + 1;                     // Bottom right

            // Second triangle of tile
            model->indices.items[index++] = vertex_idx + 1;                         // Bottom right
            model->indices.items[index++] = vertex_idx + num_vertices_per_side;     // Top left
            model->indices.items[index++] = vertex_idx + num_vertices_per_side + 1; // Top right
        }
    }

    return model;
}

void print_cube(const GEOM_OBJECT* cube) {}

void print_geom_vertex(const GEOM_VERTEX* vertices, uint32_t verticesLen) {}
void printTrzVertex(const TrzVertex* vertices, uint32_t verticesLen) {}
// Cube

Model3D* create_quad(const GEOM_VERTEX_POSITION* const globalCoords, uint32_t size)
{
    Model3D* model = (Model3D*) malloc(sizeof(Model3D));

    // Initialize meshManager with a single mesh
    model->meshManager.pMesh                = (Mesh3D*) malloc(sizeof(Mesh3D));
    model->meshManager.meshesLen            = 1;
    model->meshManager.totalMeshVerticesLen = 4; // 4 vertices for a quad

    // Initialize the single mesh
    model->meshManager.pMesh->pVertice    = (GEOM_VERTEX*) malloc(sizeof(GEOM_VERTEX) * 4);
    model->meshManager.pMesh->verticesLen = 4;

    // Initialize indices array
    model->indices.items    = (uint32_t*) malloc(sizeof(uint32_t) * 6); // 6 indices for 2 triangles
    model->indices.itemsLen = 6;

    // Set defaults
    model->vertexOffset  = 0;
    model->indiceOffset  = 0;
    model->instanceCount = 1;
    model->pChild        = NULL;
    model->childLen      = 0;

    float s  = size / 2.0f;
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

    // Copy vertices to mesh
    memcpy(model->meshManager.pMesh->pVertice, vertices, sizeof(vertices));

    // Define indices for the quad
    uint32_t indices[] = {
        0, 3, 2, 0, 2, 1, // Back face
    };

    // Copy indices to model
    memcpy(model->indices.items, indices, sizeof(indices));

    return model;
}

Model3D* create_cube(const GEOM_VERTEX_POSITION* const globalCoords, uint32_t size)
{
    Model3D* model = (Model3D*) malloc(sizeof(Model3D));

    // Initialize meshManager with a single mesh
    model->meshManager.pMesh                = (Mesh3D*) malloc(sizeof(Mesh3D));
    model->meshManager.meshesLen            = 1;
    model->meshManager.totalMeshVerticesLen = 8; // 8 vertices for cube

    // Initialize the single mesh
    model->meshManager.pMesh->pVertice    = (GEOM_VERTEX*) malloc(sizeof(GEOM_VERTEX) * 8);
    model->meshManager.pMesh->verticesLen = 8;

    // Initialize indices array
    model->indices.items    = (uint32_t*) malloc(sizeof(uint32_t) * 36); // 6 faces * 2 triangles * 3 vertices
    model->indices.itemsLen = 36;

    // Set defaults
    model->vertexOffset  = 0;
    model->indiceOffset  = 0;
    model->instanceCount = 1;
    model->pChild        = NULL;
    model->childLen      = 0;

    float s  = size / 2.0f;
    float cx = globalCoords->x;
    float cy = globalCoords->y;
    float cz = globalCoords->z;

    // Define vertices
    GEOM_VERTEX vertices[8] = {
        {cx - s, cy - s, cz - s, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1}, // 0: Left  Bottom Back
        {cx + s, cy - s, cz - s, 1, 0, 0, 1, 1, 0},                      // 1: Right Bottom Back
        {cx + s, cy + s, cz - s, 1, 1, 0, 1, 1, 1},                      // 2: Right Top    Back
        {cx - s, cy + s, cz - s, 0, 1, 0, 0, 0, 1},                      // 3: Left  Top    Back
        {cx - s, cy - s, cz + s, 0, 0, 1, 0, 0, 0},                      // 4: Left  Bottom Front
        {cx + s, cy - s, cz + s, 1, 0, 1, 1, 1, 0},                      // 5: Right Bottom Front
        {cx + s, cy + s, cz + s, 1, 1, 1, 1, 1, 1},                      // 6: Right Top    Front
        {cx - s, cy + s, cz + s, 0, 1, 1, 0, 0, 1}                       // 7: Left  Top    Front
    };

    // Copy vertices to mesh
    memcpy(model->meshManager.pMesh->pVertice, vertices, sizeof(vertices));

    // Define indices for all 6 faces
    uint32_t indices[] = {
        4, 5, 6, 6, 7, 4, // Front face
        0, 3, 2, 2, 1, 0, // Back face
        3, 7, 6, 6, 2, 3, // Top face
        0, 1, 5, 5, 4, 0, // Bottom face
        1, 2, 6, 6, 5, 1, // Right face
        0, 4, 7, 7, 3, 0  // Left face
    };

    // Copy indices to model
    memcpy(model->indices.items, indices, sizeof(indices));

    return model;
}

Model3D* create_cube_no_vertex_optimization(const GEOM_VERTEX_POSITION* const globalCoords, float size)
{
    Model3D* model = (Model3D*) malloc(sizeof(Model3D));

    // Initialize meshManager with a single mesh
    model->meshManager.pMesh                = (Mesh3D*) malloc(sizeof(Mesh3D));
    model->meshManager.meshesLen            = 1;
    model->meshManager.totalMeshVerticesLen = 36; // 36 vertices for cube (12 triangles × 3 vertices)

    // Initialize the single mesh
    model->meshManager.pMesh->pVertice    = (GEOM_VERTEX*) malloc(sizeof(GEOM_VERTEX) * 36);
    model->meshManager.pMesh->verticesLen = 36;

    // No indices needed since each vertex is unique
    model->indices.items    = NULL;
    model->indices.itemsLen = 0;

    // Set defaults
    model->vertexOffset  = 0;
    model->indiceOffset  = 0;
    model->instanceCount = 1;
    model->pChild        = NULL;
    model->childLen      = 0;

    float s  = size / 2.0f;
    float cx = globalCoords->x;
    float cy = globalCoords->y;
    float cz = globalCoords->z;

    // Define the 8 corner positions
    GEOM_VERTEX_POSITION corners[8] = {
        {cx - s, cy - s, cz - s}, // 0: Left  Bottom Back
        {cx + s, cy - s, cz - s}, // 1: Right Bottom Back
        {cx + s, cy + s, cz - s}, // 2: Right Top    Back
        {cx - s, cy + s, cz - s}, // 3: Left  Top    Back
        {cx - s, cy - s, cz + s}, // 4: Left  Bottom Front
        {cx + s, cy - s, cz + s}, // 5: Right Bottom Front
        {cx + s, cy + s, cz + s}, // 6: Right Top    Front
        {cx - s, cy + s, cz + s}  // 7: Left  Top    Front
    };

    // Define triangle indices (same as before, but now used to create vertices)
    uint32_t triangleIndices[] = {
        4, 5, 6, 6, 7, 4, // Front face
        0, 3, 2, 2, 1, 0, // Back face
        3, 7, 6, 6, 2, 3, // Top face
        0, 1, 5, 5, 4, 0, // Bottom face
        1, 2, 6, 6, 5, 1, // Right face
        0, 4, 7, 7, 3, 0  // Left face
    };

    // Barycentric coordinates for each vertex in a triangle
    GEOM_VERTEX_POSITION baryCoords[3] = {
        {1.0f, 0.0f, 0.0f}, // First vertex of triangle
        {0.0f, 1.0f, 0.0f}, // Second vertex of triangle
        {0.0f, 0.0f, 1.0f}  // Third vertex of triangle
    };

    // Create 36 vertices (12 triangles × 3 vertices each)
    for (int i = 0; i < 36; i++)
    {
        uint32_t cornerIndex = triangleIndices[i];
        uint32_t baryIndex   = i % 3; // Cycle through 0, 1, 2 for each triangle

        GEOM_VERTEX* vertex = &model->meshManager.pMesh->pVertice[i];

        // Set position from corner
        vertex->point.x = corners[cornerIndex].x;
        vertex->point.y = corners[cornerIndex].y;
        vertex->point.z = corners[cornerIndex].z;

        // Set barycentric coordinates
        vertex->bary.x = baryCoords[baryIndex].x;
        vertex->bary.y = baryCoords[baryIndex].y;
        vertex->bary.z = baryCoords[baryIndex].z;

        // Set default color (you can customize this)
        vertex->color.r = 1.0f;
        vertex->color.g = 1.0f;
        vertex->color.b = 1.0f;
        vertex->color.a = 1.0f;

        // Set default texture coordinates
        vertex->textCoords.u = 0.0f;
        vertex->textCoords.v = 0.0f;

        // Set default normal (you might want to calculate proper face normals)
        vertex->normal.x = 0.0f;
        vertex->normal.y = 0.0f;
        vertex->normal.z = 1.0f;
    }

    return model;
}

Model3D* create_sphere(const GEOM_VERTEX_POSITION* const globalCoords, uint32_t radius, uint32_t segments)
{
    // Calculate vertex and index counts
    uint32_t num_rings    = segments;
    uint32_t num_sectors  = segments;
    uint32_t num_vertices = (num_rings + 1) * (num_sectors + 1);
    uint32_t num_indices  = num_rings * num_sectors * 6;

    Model3D* model = (Model3D*) malloc(sizeof(Model3D));

    // Initialize meshManager with a single mesh
    model->meshManager.pMesh                = (Mesh3D*) malloc(sizeof(Mesh3D));
    model->meshManager.meshesLen            = 1;
    model->meshManager.totalMeshVerticesLen = num_vertices;

    // Initialize the single mesh
    model->meshManager.pMesh->pVertice    = (GEOM_VERTEX*) malloc(sizeof(GEOM_VERTEX) * num_vertices);
    model->meshManager.pMesh->verticesLen = num_vertices;

    // Initialize indices array
    model->indices.items    = (uint32_t*) malloc(sizeof(uint32_t) * num_indices);
    model->indices.itemsLen = num_indices;

    // Set defaults
    model->vertexOffset  = 0;
    model->indiceOffset  = 0;
    model->instanceCount = 1;
    model->pChild        = NULL;
    model->childLen      = 0;

    float cx = globalCoords->x;
    float cy = globalCoords->y;
    float cz = globalCoords->z;

    // Generate vertices
    uint32_t vertex_index = 0;
    for (uint32_t ring = 0; ring <= num_rings; ring++)
    {
        float theta     = ring * GLM_PI / num_rings;
        float sin_theta = sinf(theta);
        float cos_theta = cosf(theta);

        for (uint32_t sector = 0; sector <= num_sectors; sector++)
        {
            float phi     = sector * 2 * GLM_PI / num_sectors;
            float sin_phi = sinf(phi);
            float cos_phi = cosf(phi);

            // Position
            float x = cx + radius * sin_theta * cos_phi;
            float y = cy + radius * sin_theta * sin_phi;
            float z = cz + radius * cos_theta;

            // Normal (normalized position from center)
            float nx = sin_theta * cos_phi;
            float ny = sin_theta * sin_phi;
            float nz = cos_theta;

            // UV coordinates
            float u = (float) sector / num_sectors;
            float v = (float) ring / num_rings;

            GEOM_VERTEX* vertex = &model->meshManager.pMesh->pVertice[vertex_index];

            // Position
            vertex->point.x = x;
            vertex->point.y = y;
            vertex->point.z = z;

            // Normal
            vertex->normal.x = nx;
            vertex->normal.y = ny;
            vertex->normal.z = nz;

            // Texture coordinates
            vertex->textCoords.u = u;
            vertex->textCoords.v = v;
            vertex->textCoords.w = 0.0f;

            // Default color (white)
            vertex->color.r = 1.0f;
            vertex->color.g = 1.0f;
            vertex->color.b = 1.0f;
            vertex->color.a = 1.0f;

            vertex_index++;
        }
    }

    // Generate indices
    uint32_t index = 0;
    for (uint32_t ring = 0; ring < num_rings; ring++)
    {
        uint32_t ring_start      = ring * (num_sectors + 1);
        uint32_t next_ring_start = (ring + 1) * (num_sectors + 1);

        for (uint32_t sector = 0; sector < num_sectors; sector++)
        {
            uint32_t current        = ring_start + sector;
            uint32_t next           = current + 1;
            uint32_t next_ring      = next_ring_start + sector;
            uint32_t next_ring_next = next_ring + 1;

            // First triangle
            model->indices.items[index++] = current;
            model->indices.items[index++] = next_ring;
            model->indices.items[index++] = next;

            // Second triangle
            model->indices.items[index++] = next;
            model->indices.items[index++] = next_ring;
            model->indices.items[index++] = next_ring_next;
        }
    }

    return model;
}
