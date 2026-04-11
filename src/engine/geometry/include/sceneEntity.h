#pragma once

#include <stdint.h>

// Kind tag for the shared preorder entity index. Each entity in
// TransformManager.pPreorderEntities[] carries one of these values to
// identify which per-type dense array its backing instance lives in.
typedef enum SceneEntityKind
{
    SCENE_ENTITY_MODEL = 0,
    SCENE_ENTITY_LIGHT = 1,
} SceneEntityKind;

// Cold-path reverse index: given a preorder transform position, find the
// backing Model3DInstance or LightVolumeInstance.
//
// Populated at scene load time alongside pTransformsHot / pTransformsCold.
// Hot-path systems (resolve, cull, UBO fill) do NOT read this. It exists for
// subtree cascade operations, debug tooling, serialization, and picking.
//
// Size: 8 B per entry, packed.
typedef struct PreorderEntity
{
    uint16_t kind;     // SceneEntityKind
    uint16_t typeIdx;  // index into Models3DManager.pModel[] or LightManager.pLight[]
    uint32_t denseIdx; // index into pDenseModelInstances[] or pDenseLightInstances[]
} PreorderEntity;
