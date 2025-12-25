#pragma once
#include "vertex.h"
#include "particle.h"
#include "model.h"
GEOM_VERTEX* create_vertex(float x, float y, float z, float r, float g, float b, float u, float v);
GEOM_OBJECT* create_pyramid(float base_size, float height);
GEOM_OBJECT* create_rhomboid(float width, float height, float depth, float skew);
Model3D*     create_cube(const GEOM_VERTEX_POSITION* const global, uint32_t size);
Model3D*     create_cube_no_vertex_optimization(const GEOM_VERTEX_POSITION* const global, float size);
Model3D*     create_quad(const GEOM_VERTEX_POSITION* const global, uint32_t size);
GEOM_OBJECT* create_grid(GEOM_OBJECT** geometry, int numPoints, float padding);
Model3D*     create_plane(const GEOM_VERTEX_POSITION* const global, uint32_t size);
void         print_geom_vertex(const GEOM_VERTEX* vertices, uint32_t verticesLen);
void         printTrzVertex(const TrzVertex* vertices, uint32_t verticesLen);
// Primitive log functions
void printParticle(const ParticleInstance* particle);
