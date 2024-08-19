#pragma once
#include <stdint.h>
#include "cglm.h"
#include "numberTypes.h"
#include "vertex.h"

typedef struct SCENE {
    GEOM_OBJECT* pScene;
    uint32_t sceneLen;

    /// @brief indices for indexed draw of the whole scene and also vertex, should be sync with pScene state
    uint32_t totalIndicesLen;
    uint32_t* sceneIndices;

    GEOM_VERTEX* sceneVertex;
    uint32_t sceneVertexLen;

    // lights
    // effects
} SCENE;


uint32_t getIndicesLenFromScene(const SCENE const *scene);
uint32_t* getIndicesFromScene(const SCENE const *scene);
uint32_t getVertexLenFromScene(const SCENE const *scene);
GEOM_VERTEX* getVertexFromScene(const SCENE const *scene);
