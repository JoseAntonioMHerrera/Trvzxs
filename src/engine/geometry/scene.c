#include <stdlib.h>
#include <math.h>
#include "vertex.h"
#include "scene.h"

uint32_t getIndicesLenFromScene(const SCENE const *scene) {
    uint32_t indicesLen;
    for(int i = 0; i < scene->sceneLen; i++){
        indicesLen += scene->pScene[i].indicesLen;
    }
    return indicesLen;
}
