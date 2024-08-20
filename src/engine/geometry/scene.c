#include <stdlib.h>
#include <math.h>
#include "vertex.h"
#include "scene.h"



void initIndicesScene(uint32_t* indices, uint32_t indicesLen){
    for(int i=0; i < indicesLen; i++){
        indices[i] = 0;
    }
}

uint32_t getIndicesLenFromScene(const SCENE * const scene) {
    uint32_t indicesLen = 0;
    for(int i = 0; i < scene->sceneLen; i++){
        indicesLen += scene->pScene[i].indicesLen;
    }

    printf("RETORNAMOS %d INDICES LEN\n", indicesLen);
    return indicesLen;
}

uint32_t* getIndicesFromScene(const SCENE * const scene) {
    assert(scene->totalIndicesLen > 0);
    uint32_t *indices = malloc(sizeof(uint32_t) * scene->totalIndicesLen);
    initIndicesScene(indices, scene->totalIndicesLen);
    uint32_t indicesStorageIterator = 0;

    // Copy individual array of objects indices to scene indices array
    uint32_t iteratorStorageIndices = 0;
    uint32_t totalSceneObjects = scene->sceneLen;

    for(int i = 0; i < scene->sceneLen; i++){
        uint32_t currentObjectIndicesLen = scene->pScene[i].indicesLen;
        uint32_t startPositionOffset = iteratorStorageIndices; // flag current offset
        uint32_t iteratorReadIndices = 0;
        for(iteratorStorageIndices ; iteratorStorageIndices < (startPositionOffset + currentObjectIndicesLen)  ; iteratorStorageIndices++){
            printf("ITERATOR:%d\n", indicesStorageIterator);
            indices[indicesStorageIterator] = scene->pScene[i].indices[iteratorReadIndices];
            indicesStorageIterator++;
            iteratorReadIndices++;
        }
    }

    for(int i = 0; i < indicesStorageIterator; i++) {
        printf("%d\n", indices[i]);
    }

    return indices;
}

uint32_t getVertexLenFromScene(const SCENE * const scene) {
    uint32_t dataLen = 0;
    for(int i = 0; i < scene->sceneLen; i++){
            printf("AÑADIMOS %d DATA LEN\n", scene->pScene[i].dataLen);
        dataLen += scene->pScene[i].dataLen;
    }

    printf("RETORNAMOS %d DATA LEN\n", dataLen);

    return dataLen;
}

GEOM_VERTEX* getVertexFromScene(const SCENE * const scene) {
    assert(scene->sceneVertexLen > 0);
    GEOM_VERTEX *pVertex = malloc(sizeof(GEOM_VERTEX) * scene->sceneVertexLen);
    uint32_t indicesStorageIterator = 0;

    // Copy individual array of objects indices to scene indices array
    for(int i = 0; i < scene->sceneLen; i++){
        for(int indexObjVertex = 0 ; indexObjVertex <  scene->pScene[i].dataLen; indexObjVertex++){
            printf("ITERATOR:%d\n", indicesStorageIterator);
            pVertex[indicesStorageIterator] = scene->pScene[i].data[indexObjVertex];
            printf("ITERATOR:%d %d %d\n", pVertex[indexObjVertex].point.x,pVertex[indexObjVertex].point.y,pVertex[indexObjVertex].point.z);
            indicesStorageIterator++;
        }
    }

    for(int i = 0; i < scene->sceneVertexLen; i++) {
        printf("X: %.6f\n", pVertex[i].point.x);
        printf("Y: %f\n", pVertex[i].point.y);
        printf("Z: %f\n", pVertex[i].point.z);
    }

    return pVertex;
}
