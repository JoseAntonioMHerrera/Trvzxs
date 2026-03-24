#define FAST_OBJ_IMPLEMENTATION
#include "fast_obj.h"
#include "vertex.h"
#include "model.h"

Model3D* load_obj_to_geom_object(const char* filename)
{

    fastObjMesh* mesh = fast_obj_read(filename);
    if (!mesh)
    {
        return NULL;
    }

    Model3D* model = (Model3D*) malloc(sizeof(Model3D));
    if (!model)
    {
        fast_obj_destroy(mesh);
        return NULL;
    }

    // Initialize mesh manager with single mesh
    model->meshManager.pMesh = (Mesh3D*) malloc(sizeof(Mesh3D));
    if (!model->meshManager.pMesh)
    {
        free(model);
        fast_obj_destroy(mesh);
        return NULL;
    }
    model->meshManager.meshesLen = 1;

    // Allocate vertex data
    uint32_t total_vertices = mesh->face_count * 3; // Assuming triangles

    model->meshManager.pMesh->verticesLen   = total_vertices;
    model->meshManager.totalMeshVerticesLen = total_vertices;
    model->meshManager.pMesh->pVertice      = (GEOM_VERTEX*) malloc(total_vertices * sizeof(GEOM_VERTEX));
    if (!model->meshManager.pMesh->pVertice)
    {
        free(model->meshManager.pMesh);
        free(model);
        fast_obj_destroy(mesh);
        return NULL;
    }

    // Allocate index data
    model->indices.itemsLen = mesh->face_count * 3;
    model->indices.items    = (uint32_t*) malloc(model->indices.itemsLen * sizeof(uint32_t));
    if (!model->indices.items)
    {
        free(model->meshManager.pMesh->pVertice);
        free(model->meshManager.pMesh);
        free(model);
        fast_obj_destroy(mesh);
        return NULL;
    }

    // Set defaults
    model->vertexOffset   = 0;
    model->indiceOffset   = 0;
    model->instanceCount  = 0;
    model->pChild         = NULL;
    model->childLen       = 0;
    model->pModelInstance = 0;

    // Convert mesh data to Model3D format
    uint32_t vertex_count         = 0;
    uint32_t faces_with_normals   = 0;
    uint32_t faces_with_texcoords = 0;

    for (unsigned int i = 0; i < mesh->face_count; i++)
    {

        bool face_has_normals   = false;
        bool face_has_texcoords = false;

        for (unsigned int j = 0; j < 3; j++)
        { // Assuming triangles
            fastObjIndex idx = mesh->indices[i * 3 + j];

            // Position
            model->meshManager.pMesh->pVertice[vertex_count].point.x = mesh->positions[3 * idx.p];
            model->meshManager.pMesh->pVertice[vertex_count].point.y = mesh->positions[3 * idx.p + 1];
            model->meshManager.pMesh->pVertice[vertex_count].point.z = mesh->positions[3 * idx.p + 2];

            // Normal (if available)
            if (idx.n != 0)
            {
                model->meshManager.pMesh->pVertice[vertex_count].normal.x = mesh->normals[3 * idx.n];
                model->meshManager.pMesh->pVertice[vertex_count].normal.y = mesh->normals[3 * idx.n + 1];
                model->meshManager.pMesh->pVertice[vertex_count].normal.z = mesh->normals[3 * idx.n + 2];
                face_has_normals                                          = true;
            }
            else
            {
                // Default normal if not provided
                model->meshManager.pMesh->pVertice[vertex_count].normal.x = 0.0f;
                model->meshManager.pMesh->pVertice[vertex_count].normal.y = 0.0f;
                model->meshManager.pMesh->pVertice[vertex_count].normal.z = 1.0f;
            }

            // Texture coordinates (if available)
            if (idx.t != 0)
            {
                model->meshManager.pMesh->pVertice[vertex_count].textCoords.u = mesh->texcoords[2 * idx.t];
                model->meshManager.pMesh->pVertice[vertex_count].textCoords.v = 1 - mesh->texcoords[2 * idx.t + 1];
                model->meshManager.pMesh->pVertice[vertex_count].textCoords.w = 0.0f;
                face_has_texcoords                                            = true;
            }
            else
            {
                model->meshManager.pMesh->pVertice[vertex_count].textCoords.u = 0.0f;
                model->meshManager.pMesh->pVertice[vertex_count].textCoords.v = 0.0f;
                model->meshManager.pMesh->pVertice[vertex_count].textCoords.w = 0.0f;
            }

            // Set default color
            model->meshManager.pMesh->pVertice[vertex_count].color.r = 1.0f;
            model->meshManager.pMesh->pVertice[vertex_count].color.g = 1.0f;
            model->meshManager.pMesh->pVertice[vertex_count].color.b = 1.0f;
            model->meshManager.pMesh->pVertice[vertex_count].color.a = 1.0f;

            // Set index
            model->indices.items[vertex_count] = vertex_count;

            vertex_count++;
        }

        if (face_has_normals)
            faces_with_normals++;
        if (face_has_texcoords)
            faces_with_texcoords++;
    }

    // Clean up
    fast_obj_destroy(mesh);

    return model;
}

void free_geom_object(GEOM_OBJECT* obj)
{
    if (obj)
    {
        free(obj->data);
        free(obj->indices);
        free(obj);
    }
}
