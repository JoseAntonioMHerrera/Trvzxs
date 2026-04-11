#pragma once

#include <stdint.h>
#include "cglm.h"
#include "sceneEntity.h"
#include "vertex.h"

// A node in the declarative scene spec tree. Each SceneEntitySpec describes
// one instance (model or light) with its TRS + color and an array of child
// specs. Passed to fillSceneFromSpecTree() at scene load time.
//
// The spec tree is owned by the caller; after fillSceneFromSpecTree() copies
// the data into the engine's dense arrays, the caller is responsible for
// freeing the tree via sceneSpecFree().
typedef struct SceneEntitySpec
{
    SceneEntityKind kind;
    uint32_t        typeIdx;  // index into Models3DManager.pModel[] or LightManager.pLight[]
    vec3            position;
    versor          rotation;
    vec3            scale;
    GEOM_VERTEX_COLOR_RGBA color;

    struct SceneEntitySpec** children;      // array of child spec pointers
    uint32_t                 childCount;    // number of children
    uint32_t                 childCapacity; // allocated slots in children
} SceneEntitySpec;

// --- Builder helpers ---

// Allocate a new model spec. Rotation defaults to identity, scale to (1,1,1).
SceneEntitySpec* sceneSpecNewModel(uint32_t typeIdx, vec3 position, GEOM_VERTEX_COLOR_RGBA color);

// Allocate a new light spec with a uniform scale (the common case for light volumes).
SceneEntitySpec* sceneSpecNewLight(uint32_t typeIdx, vec3 position, float uniformScale,
                                   GEOM_VERTEX_COLOR_RGBA color);

// Append a child to a parent. Ownership of the child transfers to the parent —
// sceneSpecFree() on the parent will recursively free the child.
void sceneSpecAddChild(SceneEntitySpec* parent, SceneEntitySpec* child);

// Recursively free a spec tree.
void sceneSpecFree(SceneEntitySpec* root);

// --- Scene fill ---

typedef struct SCENE SCENE;

// Populate transform arrays, per-type dense instance arrays, preorder entity
// index, and local AABBs from a spec tree.
//
// Preconditions:
//   - scene->modelsManager.pModel[] and scene->lightManager.pLight[] are
//     already populated with type metadata, including each type's
//     pre-computed localAABB.
//   - scene->modelsManager.modelLen and scene->lightManager.lightLen are set.
//   - scene->modelsManager.pDenseModelInstances and scene->lightManager.
//     pDenseLightInstances are NULL (this function allocates them).
//   - scene->transformManager's arrays are NULL (this function allocates them).
//
// Postconditions:
//   - pTransformsHot / pTransformsCold / pPreorderEntities / pLocalAABBs are
//     populated in DFS preorder. Each TransformHot.parentIdx satisfies
//     parentIdx < childIdx or equals TRANSFORM_NO_PARENT.
//   - TransformHot.skipCount on each node equals the size of its subtree
//     (0 for leaves).
//   - Per-type Model3D.denseBaseOffset / instanceCount and LightVolume.
//     denseBaseOffset / lightVolumeInstanceCount are populated based on the
//     spec-tree counts.
//   - Each Model3DInstance.transformIdx and LightVolumeInstance.transformIdx
//     points to the matching preorder slot.
void fillSceneFromSpecTree(SCENE* scene, SceneEntitySpec** roots, uint32_t rootCount);
