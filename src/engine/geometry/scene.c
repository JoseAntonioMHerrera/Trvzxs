#include <stdlib.h>
#include <math.h>
#include "vertex.h"
#include "scene.h"


void apply_offset_to_indices(TRVZXS_SIZE* indices, size_t num_indices, TRVZXS_SIZE offset) {
    for (size_t i = 0; i < num_indices; i++) {
        indices[i] += offset;
    }
}

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

    //printf("RETORNAMOS %d INDICES LEN\n", indicesLen);
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
            //printf("ITERATOR:%d\n", indicesStorageIterator);
            indices[indicesStorageIterator] = scene->pScene[i].indices[iteratorReadIndices];
            indicesStorageIterator++;
            iteratorReadIndices++;
        }
    }


    return indices;
}

uint32_t getVertexLenFromScene(const SCENE * const scene) {
    uint32_t dataLen = 0;
    for(int i = 0; i < scene->sceneLen; i++){
            //printf("AÑADIMOS %d DATA LEN\n", scene->pScene[i].dataLen);
        dataLen += scene->pScene[i].dataLen;
    }

    //printf("RETORNAMOS %d DATA LEN\n", dataLen);

    return dataLen;
}

GEOM_VERTEX* getVertexFromScene(const SCENE * const scene) {
    assert(scene->sceneVertexLen > 0);
    GEOM_VERTEX *pVertex = malloc(sizeof(GEOM_VERTEX) * scene->sceneVertexLen);
    uint32_t indicesStorageIterator = 0;

    // Copy individual array of objects indices to scene indices array
    for(int i = 0; i < scene->sceneLen; i++){
        for(int indexObjVertex = 0 ; indexObjVertex <  scene->pScene[i].dataLen; indexObjVertex++){
            //printf("ITERATOR:%d\n", indicesStorageIterator);
            pVertex[indicesStorageIterator] = scene->pScene[i].data[indexObjVertex];
            //printf("ITERATOR:%d %d %d\n", pVertex[indexObjVertex].point.x,pVertex[indexObjVertex].point.y,pVertex[indexObjVertex].point.z);
            indicesStorageIterator++;
        }
    }

    return pVertex;
}

void initScene(SCENE*** scene, uint32_t sceneLen){
    (**scene) = (SCENE*) malloc(sizeof(SCENE));
    (**scene)->pScene = (GEOM_OBJECT*) malloc(sizeof(GEOM_OBJECT) * sceneLen);
    (**scene)->sceneLen = sceneLen;
}

void printScene(GEOM_OBJECT** objects, uint32_t objectsLen){
    //printf("OBJECTS: %x -> %d\n",objects, objectsLen);
    for(int objIndex = 0; objIndex < objectsLen; objIndex++){
        for(int vertexIndex = 0; vertexIndex < objects[objIndex]->dataLen; vertexIndex++)
            printf("(X,Y,Z):{%f,%f,%f} \n", objects[objIndex]->data[vertexIndex].point.x,
            objects[objIndex]->data[vertexIndex].point.y,
            objects[objIndex]->data[vertexIndex].point.z);
    }
};

void printScene_(GEOM_OBJECT* objects, uint32_t objectsLen){
    //printf("OBJECTS: %x -> %d\n",objects, objectsLen);
    for(int objIndex = 0; objIndex < objectsLen; objIndex++){
        for(int vertexIndex = 0; vertexIndex < objects[objIndex].dataLen; vertexIndex++)
            printf("(X,Y,Z):{%f,%f,%f} \n", objects[objIndex].data[vertexIndex].point.x,
            objects[objIndex].data[vertexIndex].point.y,
            objects[objIndex].data[vertexIndex].point.z);
    }
};

void initSceneWithData(SCENE** scene, GEOM_OBJECT** objects, uint32_t sceneLen){
    printf("INIT SCENE WITH SCENELEN:%d\n", sceneLen);
    uint32_t spaceOffsetForModel = 0;
    uint32_t indexOffsetForModelIndices = 0;
    initScene(&scene, sceneLen);
    //printScene(objects, sceneLen);

    for(int objIndex = 0; objIndex < sceneLen; objIndex++){
        (*scene)->pScene[objIndex] = *objects[objIndex];
        //printf("SDFUNSOGNOSD\n");
        GEOM_OBJECT* cube = &(*scene)->pScene[objIndex];

        apply_offset_to_indices(cube->indices, cube->indicesLen, indexOffsetForModelIndices);
        indexOffsetForModelIndices += cube->vertexOffset;

        //printf("INDICES DEL CUBO: %d\n", (*scene)->pScene[objIndex].indicesLen);
        spaceOffsetForModel+=5;
    }

    //printf("TRAS ITERAR EL BUCLE DE OBJETOS\n");

    (*scene)->sceneVertexLen =  getVertexLenFromScene((*scene));
    (*scene)->sceneVertex =  getVertexFromScene((*scene));

    (*scene)->totalIndicesLen = getIndicesLenFromScene((*scene));
    (*scene)->sceneIndices = getIndicesFromScene((*scene));
    //printf("FINALIZAMOS CREACION ESCENA!\n");
    //printf("SCENELEN : %d", (*scene)->sceneLen);
    //printf("INDICESLEN : %d", (*scene)->totalIndicesLen);
}
