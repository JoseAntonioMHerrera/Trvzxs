#pragma once
#include "array.h"
#include "transformations.h"
#include "vertex.h"

typedef struct Model3D Model3D;
typedef struct Mesh3D
{
    GEOM_VERTEX* pVertice;
    uint32_t     verticesLen;
} Mesh3D;
typedef struct Mesh3DManager
{
    Mesh3D*  pMesh;
    uint32_t meshesLen;
    uint32_t totalMeshVerticesLen;
    // vec4     boundingBox[2];
} Mesh3DManager;
// Instance-level alive flag values
#define INSTANCE_ALIVE     1
#define INSTANCE_DISABLED  0
#define INSTANCE_DESTROYED 2

// Model-level alive flag values
#define MODEL_ALIVE     1
#define MODEL_DISABLED  2
#define MODEL_DESTROYED 0

typedef struct Model3DInstance
{
    mat4                   m;
    GEOM_VERTEX_COLOR_RGBA color;
    uint32_t               alive;
} Model3DInstance;

typedef struct Model3D
{
    Mesh3DManager meshManager;
    ArrayUint32_t indices;

    uint32_t vertexOffset;
    uint32_t indiceOffset;
    uint32_t instanceCount; // Total allocated instances (capacity), immutable after init
    uint32_t instanceIndex;

    // Number of live instances. Computed each frame by the UBO fill loop —
    // not maintained by toggle functions. Read by the draw loop.
    uint32_t liveInstanceCount;

    // Model-level lifecycle: 1 = alive, 2 = disabled, 0 = permanently destroyed.
    // Fill loop skips the entire model when alive != 1.
    uint32_t alive;

    Model3DInstance* pModelInstance;
    uint32_t         modelInstanceCount;
    Model3D*         pChild;
    uint32_t         childLen;
} Model3D;

typedef struct Models3DManager
{
    Model3D* pModel;
    uint32_t modelLen;
    uint32_t modelInstanceLen;

    GEOM_VERTEX* pPackedModelVertex;
    uint32_t     packedModelVertexLen;

    ArrayUint32_t packedModelIndex;

    ArrayUint32_t instanceOffset;
} Models3DManager;

Model3D* load_obj_to_geom_object(const char* filename);
void     freeModel3D(Model3D* model);
void     freeMesh(Mesh3D* mesh);
void     freeMeshManager(Mesh3DManager* meshManager);
// void     getModelInstancesCount(const Models3DManager const* manager);
