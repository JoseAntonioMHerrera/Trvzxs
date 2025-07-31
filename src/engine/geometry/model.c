#define FAST_OBJ_IMPLEMENTATION
#include "fast_obj.h"
#include "vertex.h"
#include "model.h"

Model3D* load_obj_to_geom_object(const char* filename)
{
    printf("DEBUG: Starting OBJ loading for file: %s\n", filename);

    fastObjMesh* mesh = fast_obj_read(filename);
    if (!mesh)
    {
        printf("DEBUG: ERROR - Failed to read OBJ file: %s\n", filename);
        return NULL;
    }

    printf("DEBUG: OBJ file loaded successfully\n");
    printf("DEBUG: Face count: %u\n", mesh->face_count);
    printf("DEBUG: Position count: %u\n", mesh->position_count);
    printf("DEBUG: Normal count: %u\n", mesh->normal_count);
    printf("DEBUG: Texcoord count: %u\n", mesh->texcoord_count);

    Model3D* model = (Model3D*) malloc(sizeof(Model3D));
    if (!model)
    {
        printf("DEBUG: ERROR - Failed to allocate memory for Model3D\n");
        fast_obj_destroy(mesh);
        return NULL;
    }
    printf("DEBUG: Model3D allocated successfully\n");

    // Initialize mesh manager with single mesh
    model->meshManager.pMesh = (Mesh3D*) malloc(sizeof(Mesh3D));
    if (!model->meshManager.pMesh)
    {
        printf("DEBUG: ERROR - Failed to allocate memory for Mesh3D\n");
        free(model);
        fast_obj_destroy(mesh);
        return NULL;
    }
    model->meshManager.meshesLen = 1;
    printf("DEBUG: Mesh3D allocated successfully\n");

    // Allocate vertex data
    uint32_t total_vertices = mesh->face_count * 3; // Assuming triangles
    printf("DEBUG: Calculating total vertices: %u faces * 3 = %u vertices\n", mesh->face_count, total_vertices);

    model->meshManager.pMesh->verticesLen   = total_vertices;
    model->meshManager.totalMeshVerticesLen = total_vertices;
    model->meshManager.pMesh->pVertice      = (GEOM_VERTEX*) malloc(total_vertices * sizeof(GEOM_VERTEX));
    if (!model->meshManager.pMesh->pVertice)
    {
        printf("DEBUG: ERROR - Failed to allocate memory for vertices (%u vertices, %zu bytes)\n", total_vertices,
               total_vertices * sizeof(GEOM_VERTEX));
        free(model->meshManager.pMesh);
        free(model);
        fast_obj_destroy(mesh);
        return NULL;
    }
    printf("DEBUG: Vertex array allocated successfully (%zu bytes)\n", total_vertices * sizeof(GEOM_VERTEX));

    // Allocate index data
    model->indices.itemsLen = mesh->face_count * 3;
    printf("DEBUG: Allocating indices: %u items\n", model->indices.itemsLen);
    model->indices.items = (uint32_t*) malloc(model->indices.itemsLen * sizeof(uint32_t));
    if (!model->indices.items)
    {
        printf("DEBUG: ERROR - Failed to allocate memory for indices (%u indices, %zu bytes)\n",
               model->indices.itemsLen, model->indices.itemsLen * sizeof(uint32_t));
        free(model->meshManager.pMesh->pVertice);
        free(model->meshManager.pMesh);
        free(model);
        fast_obj_destroy(mesh);
        return NULL;
    }
    printf("DEBUG: Index array allocated successfully (%zu bytes)\n", model->indices.itemsLen * sizeof(uint32_t));

    // Set defaults
    model->vertexOffset   = 0;
    model->indiceOffset   = 0;
    model->instanceCount  = 0;
    model->pChild         = NULL;
    model->childLen       = 0;
    model->pModelInstance = 0;
    printf("DEBUG: Default values set\n");

    // Convert mesh data to Model3D format
    printf("DEBUG: Starting vertex data conversion...\n");
    uint32_t vertex_count         = 0;
    uint32_t faces_with_normals   = 0;
    uint32_t faces_with_texcoords = 0;

    for (unsigned int i = 0; i < mesh->face_count; i++)
    {
        if (i % 1000 == 0 && i > 0)
        {
            printf("DEBUG: Processing face %u/%u (%.1f%%)\n", i, mesh->face_count,
                   (float) i / mesh->face_count * 100.0f);
        }

        bool face_has_normals   = false;
        bool face_has_texcoords = false;

        for (unsigned int j = 0; j < 3; j++)
        { // Assuming triangles
            fastObjIndex idx = mesh->indices[i * 3 + j];

            // Position
            if (idx.p >= mesh->position_count)
            {
                printf("DEBUG: WARNING - Position index %u out of range (max: %u) at face %u, vertex %u\n", idx.p,
                       mesh->position_count - 1, i, j);
            }
            model->meshManager.pMesh->pVertice[vertex_count].point.x = mesh->positions[3 * idx.p];
            model->meshManager.pMesh->pVertice[vertex_count].point.y = mesh->positions[3 * idx.p + 1];
            model->meshManager.pMesh->pVertice[vertex_count].point.z = mesh->positions[3 * idx.p + 2];

            // Normal (if available)
            if (idx.n != 0)
            {
                if (idx.n >= mesh->normal_count)
                {
                    printf("DEBUG: WARNING - Normal index %u out of range (max: %u) at face %u, vertex %u\n", idx.n,
                           mesh->normal_count - 1, i, j);
                }
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
                if (idx.t >= mesh->texcoord_count)
                {
                    printf("DEBUG: WARNING - Texcoord index %u out of range (max: %u) at face %u, vertex %u\n", idx.t,
                           mesh->texcoord_count - 1, i, j);
                }
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

    printf("DEBUG: Vertex conversion completed\n");
    printf("DEBUG: Total vertices processed: %u\n", vertex_count);
    printf("DEBUG: Faces with normals: %u/%u (%.1f%%)\n", faces_with_normals, mesh->face_count,
           (float) faces_with_normals / mesh->face_count * 100.0f);
    printf("DEBUG: Faces with texture coordinates: %u/%u (%.1f%%)\n", faces_with_texcoords, mesh->face_count,
           (float) faces_with_texcoords / mesh->face_count * 100.0f);

    // Clean up
    fast_obj_destroy(mesh);
    printf("DEBUG: fastObjMesh destroyed\n");

    printf("DEBUG: OBJ loading completed successfully\n");
    printf("DEBUG: Final model stats - Vertices: %u, Indices: %u, Instance count: %u\n",
           model->meshManager.pMesh->verticesLen, model->indices.itemsLen, model->instanceCount);

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
