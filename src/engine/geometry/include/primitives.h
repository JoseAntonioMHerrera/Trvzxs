#pragma once
#include "vertex.h"

GEOM_VERTEX* create_vertex(float x, float y, float z, float r, float g, float b, float u, float v);
GEOM_OBJECT* create_pyramid(float base_size, float height);
GEOM_OBJECT* create_rhomboid(float width, float height, float depth, float skew);
GEOM_OBJECT* create_cube(const GEOM_VERTEX_POSITION* const global, uint32_t size);
GEOM_OBJECT* create_quad(const GEOM_VERTEX_POSITION* const global, uint32_t size);
GEOM_OBJECT* create_grid(GEOM_OBJECT**geometry, int numPoints, float padding);
