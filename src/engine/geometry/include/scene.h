#pragma once
#include <stdint.h>
#include "cglm.h"
#include "numberTypes.h"
#include "vertex.h"

typedef struct SCENE {
    GEOM_OBJECT* pScene;
    uint32_t sceneLen;
    uint32_t totalIndicesLen;
    // lights
    // effects
} SCENE;


uint32_t getIndicesLenFromScene(const SCENE const *scene);
