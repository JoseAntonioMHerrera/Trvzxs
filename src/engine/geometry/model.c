#define FAST_OBJ_IMPLEMENTATION
#include "fast_obj.h"
#include "vertex.h"

GEOM_OBJECT* load_obj_to_geom_object(const char* filename) {
    fastObjMesh* mesh = fast_obj_read(filename);
    if (!mesh) {
        printf("Failed to load OBJ file\n");
        return NULL;
    }

    GEOM_OBJECT* obj = (GEOM_OBJECT*)malloc(sizeof(GEOM_OBJECT));
    if (!obj) {
        fast_obj_destroy(mesh);
        return NULL;
    }

    // Allocate vertex data
    obj->dataLen = mesh->face_count * 3; // Assuming triangles
    obj->data = (GEOM_VERTEX*)malloc(obj->dataLen * sizeof(GEOM_VERTEX));
    if (!obj->data) {
        free(obj);
        fast_obj_destroy(mesh);
        return NULL;
    }

    // Allocate index data
    obj->indicesLen = mesh->face_count * 3;
    obj->indices = (TRVZXS_SIZE*)malloc(obj->indicesLen * sizeof(TRVZXS_SIZE));
    if (!obj->indices) {
        free(obj->data);
        free(obj);
        fast_obj_destroy(mesh);
        return NULL;
    }

    obj->vertexOffset = 0;

    // Convert mesh data to GEOM_OBJECT format
    TRVZXS_SIZE vertex_count = 0;
    for (unsigned int i = 0; i < mesh->face_count; i++) {
        for (unsigned int j = 0; j < 3; j++) { // Assuming triangles
            fastObjIndex idx = mesh->indices[i * 3 + j];

            // Position
            obj->data[vertex_count].point.x = mesh->positions[3 * idx.p];
            obj->data[vertex_count].point.y = mesh->positions[3 * idx.p + 1];
            obj->data[vertex_count].point.z = mesh->positions[3 * idx.p + 2];

            // Normal (if available)
            if (idx.n != 0) {
                obj->data[vertex_count].normal.x = mesh->normals[3 * idx.n];
                obj->data[vertex_count].normal.y = mesh->normals[3 * idx.n + 1];
                obj->data[vertex_count].normal.z = mesh->normals[3 * idx.n + 2];
            } else {
                // Default normal if not provided
                obj->data[vertex_count].normal.x = 0.0f;
                obj->data[vertex_count].normal.y = 0.0f;
                obj->data[vertex_count].normal.z = 1.0f;
            }

            // Texture coordinates (if available)
            if (idx.t != 0) {
                obj->data[vertex_count].textCoords.u = mesh->texcoords[2 * idx.t];
                obj->data[vertex_count].textCoords.v = 1 - mesh->texcoords[2 * idx.t + 1];
            } else {
                obj->data[vertex_count].textCoords.u = 0.0f;
                obj->data[vertex_count].textCoords.v = 0.0f;
            }

            // Set a default color if GEOM_VERTEX_COLOR_RGBA is part of your structure
            obj->data[vertex_count].color.r = 1.0f;
            obj->data[vertex_count].color.g = 1.0f;
            obj->data[vertex_count].color.b = 1.0f;
            obj->data[vertex_count].color.a = 1.0f;

            // Set index
            obj->indices[vertex_count] = vertex_count;

            vertex_count++;
        }
    }

    // Clean up
    fast_obj_destroy(mesh);

    return obj;
}

void free_geom_object(GEOM_OBJECT* obj) {
    if (obj) {
        free(obj->data);
        free(obj->indices);
        free(obj);
    }
}
