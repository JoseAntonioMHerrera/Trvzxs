#include <stdlib.h>
#include <math.h>
#include "vertex.h"
#include "scene.h"
#include "light.h"
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

uint32_t getModelIndicesLenFromScene(const SCENE * const scene) {
    uint32_t indicesLen = 0;
    for(int i = 0; i < scene->modelsManager.modelLen; i++){
        indicesLen += scene->modelsManager.pModel[i].indices.itemsLen;
    }

    printf("RETORNAMOS %d INDICES LENNNNNNNNNNNNNN\n\n\n\n\n", indicesLen);
    return indicesLen;
}

uint32_t* getModelIndicesFromScene(const SCENE * const scene) {
    assert(scene->modelsManager.packedModelIndex.itemsLen > 0);
    uint32_t *indices = malloc(sizeof(uint32_t) * scene->modelsManager.packedModelIndex.itemsLen);
    initIndicesScene(indices, scene->modelsManager.packedModelIndex.itemsLen);
    uint32_t indicesStorageIterator = 0;

    // Copy individual array of objects indices to scene indices array
    uint32_t iteratorStorageIndices = 0;
    uint32_t totalSceneObjects = scene->modelsManager.modelLen;

    for(int i = 0; i < scene->modelsManager.modelLen; i++){
        uint32_t currentObjectIndicesLen = scene->modelsManager.pModel[i].indices.itemsLen;
        uint32_t startPositionOffset = iteratorStorageIndices; // flag current offset
        uint32_t iteratorReadIndices = 0;
        for(iteratorStorageIndices ; iteratorStorageIndices < (startPositionOffset + currentObjectIndicesLen)  ; iteratorStorageIndices++){
            //printf("ITERATOR:%d\n", indicesStorageIterator);
            indices[indicesStorageIterator] = scene->modelsManager.pModel[i].indices.items[iteratorReadIndices];
            indicesStorageIterator++;
            iteratorReadIndices++;
        }
    }


    return indices;
}

uint32_t getModelVerticesLenFromScene(const SCENE * const scene) {
    uint32_t dataLen = 0;
    for(int i = 0; i < scene->modelsManager.modelLen; i++){
        for(int j = 0; j < scene->modelsManager.pModel[i].meshManager.meshesLen; j++){
            dataLen += scene->modelsManager.pModel[i].meshManager.pMesh[j].verticesLen;
            printf("AÑADIMOS VERTICES LEN EN GETMODELVERTICESLENfromScene\n");
        }

    }

    printf("RETORNAMOS %d DATA LENNNNNNNNNNNNNN\n\n\n\n\n", dataLen);
    return dataLen;
}

GEOM_VERTEX* getModelVerticesFromScene(const SCENE * const scene) {
    assert(scene->modelsManager.modelLen > 0);
    GEOM_VERTEX *pVertex = malloc(sizeof(GEOM_VERTEX) * scene->modelsManager.packedModelVertexLen);
    uint32_t indicesStorageIterator = 0;

    // Copy individual array of objects indices to scene indices array
    for(int i = 0; i < scene->modelsManager.modelLen; i++){
        for(int j = 0; j < scene->modelsManager.pModel[i].meshManager.meshesLen; j++){
            for(int indexObjVertex = 0 ; indexObjVertex <  scene->modelsManager.pModel[i].meshManager.pMesh[j].verticesLen; indexObjVertex++){
                //printf("ITERATOR:%d\n", indicesStorageIterator);
                pVertex[indicesStorageIterator] = scene->modelsManager.pModel[i].meshManager.pMesh[j].pVertice[indexObjVertex];
                //printf("ITERATOR:%d %d %d\n", pVertex[indexObjVertex].point.x,pVertex[indexObjVertex].point.y,pVertex[indexObjVertex].point.z);
                indicesStorageIterator++;
            }
        }

    }
    printf("FINALIZAMOS getModelVerticesFromScene\n");
    return pVertex;
}

uint32_t getVolumetricLightsIndicesLenFromScene(const SCENE * const scene) {
    uint32_t indicesLen = 0;
    for(int i = 0; i < scene->lightManager.lightLen; i++){
        indicesLen += scene->lightManager.pLight[i].model.indices.itemsLen;
    }

    //printf("RETORNAMOS %d INDICES LEN\n", indicesLen);
    return indicesLen;
}

uint32_t* getVolumetricLightsIndicesFromScene(const SCENE * const scene) {
    assert(scene->lightManager.packedLightIndex.itemsLen > 0);
    uint32_t *indices = malloc(sizeof(uint32_t) * scene->lightManager.packedLightIndex.itemsLen);
    initIndicesScene(indices, scene->lightManager.packedLightIndex.itemsLen);
    uint32_t indicesStorageIterator = 0;

    // Copy individual array of objects indices to scene indices array
    uint32_t iteratorStorageIndices = 0;
    uint32_t totalSceneObjects = scene->lightManager.lightLen;

    for(int i = 0; i < scene->lightManager.lightLen; i++){
        uint32_t currentObjectIndicesLen = scene->lightManager.pLight[i].model.indices.itemsLen;
        uint32_t startPositionOffset = iteratorStorageIndices; // flag current offset
        uint32_t iteratorReadIndices = 0;
        for(iteratorStorageIndices ; iteratorStorageIndices < (startPositionOffset + currentObjectIndicesLen)  ; iteratorStorageIndices++){
            //printf("ITERATOR:%d\n", indicesStorageIterator);
            indices[indicesStorageIterator] = scene->modelsManager.pModel[i].indices.items[iteratorReadIndices];
            indicesStorageIterator++;
            iteratorReadIndices++;
        }
    }

    return indices;
}

uint32_t getVolumetricLightVerticesLenFromScene(const SCENE * const scene) {
    uint32_t dataLen = 0;
    for(int i = 0; i < scene->lightManager.lightLen; i++){
        for(int j = 0; j < scene->lightManager.pLight[i].model.meshManager.meshesLen; j++){
            dataLen += scene->lightManager.pLight[i].model.meshManager.pMesh[j].verticesLen;
        }

    }
    return dataLen;
}

GEOM_VERTEX* getLightVolumesVerticesFromScene(const SCENE * const scene) {
    assert(scene->lightManager.packedLightLen > 0);
    printf("FLAG 1 getVolumetricLightsVerticesFromScene %d\n", scene->lightManager.pLight[0].model.meshManager.pMesh[0].verticesLen);
        printf("FLAG 1 lightlen %d\n", scene->lightManager.lightLen);
            printf("FLAG 1 meshlen %d\n", scene->lightManager.pLight[0].model.meshManager.meshesLen);
    GEOM_VERTEX*pVertex = malloc(sizeof(GEOM_VERTEX) * scene->lightManager.packedLightLen);
    uint32_t indicesStorageIterator = 0;
    printf("FLAG 2 getVolumetricLightsVerticesFromScene\n");

    // Copy individual array of objects indices to scene indices array
    for(int i = 0; i < scene->lightManager.lightLen; i++){
        printf("FLAG BAGGGGGGGGG\n");
        for(int j=0; j < scene->lightManager.pLight[i].model.meshManager.meshesLen; j++){
            for(int indexObjVertex = 0 ; indexObjVertex <  scene->lightManager.pLight[i].model.meshManager.pMesh[j].verticesLen; indexObjVertex++){
                    printf("FLAG 2.5 getVolumetricLightsVerticesFromScene\n");
                //printf("ITERATOR:%d\n", indicesStorageIterator);
                pVertex[indicesStorageIterator] = scene->lightManager.pLight[i].model.meshManager.pMesh[j].pVertice[indexObjVertex];
                printf("%f, %f, %f",pVertex[indicesStorageIterator].point.x, pVertex[indicesStorageIterator].point.y, pVertex[indicesStorageIterator].point.z);
                //printf("ITERATOR:%d %d %d\n", pVertex[indexObjVertex].point.x,pVertex[indexObjVertex].point.y,pVertex[indexObjVertex].point.z);
                indicesStorageIterator++;
                printf("INDEX STORAGE ITERATOR %d\n", indicesStorageIterator);
            }
        }
    }
    printf("FLAG 3 getVolumetricLightsVerticesFromScene\n");


    return pVertex;
}

/*
    SCENE** scene: pointer reference to init the memory for the scene;
    uint32_t modelsLen: number of GEOM_OBJE

*/
void initScene(SCENE** scene, uint32_t modelsLen, uint32_t lightsLen){
    (*scene) = (SCENE*) malloc(sizeof(SCENE));
    if(modelsLen <= 0){
        (*scene)->modelsManager.modelLen = 0;
    }else{
        (*scene)->modelsManager.pModel = (Model3D*) malloc(sizeof(Model3D) * modelsLen);
        (*scene)->modelsManager.modelLen = modelsLen;
    }

    if(lightsLen <= 0){
        (*scene)->lightManager.lightLen = 0;
    }else{
        (*scene)->lightManager.pLight = (LightVolume*) malloc(sizeof(LightVolume) * lightsLen);
        (*scene)->lightManager.lightLen = lightsLen;
    }
}

void printSceneModels(const Models3DManager* const modelManager) {
    // Iterate through all models
    for (uint32_t modelIndex = 0; modelIndex < modelManager->modelLen; modelIndex++) {
        Model3D* currentModel = &modelManager->pModel[modelIndex];
        Mesh3DManager* meshManager = &currentModel->meshManager;

        // Iterate through all meshes in current model
        for (uint32_t meshIndex = 0; meshIndex < meshManager->meshesLen; meshIndex++) {
            Mesh3D* currentMesh = &meshManager->pMesh[meshIndex];

            // Iterate through all vertices in current mesh
            for (uint32_t vertexIndex = 0; vertexIndex < currentMesh->verticesLen; vertexIndex++) {
                GEOM_VERTEX vertex = currentMesh->pVertice[vertexIndex];
                printf("Model %u, Mesh %u, Vertex %u - (X,Y,Z):{%f,%f,%f}\n",
                    modelIndex,
                    meshIndex,
                    vertexIndex,
                    vertex.point.x,
                    vertex.point.y,
                    vertex.point.z);
            }
        }
    }
}

void printSceneLights(const LightManager* const volManager) {
    // Iterate through all volumetric lights
    for (uint32_t lightIndex = 0; lightIndex < volManager->lightLen; lightIndex++) {
        LightVolume* currentLight = &volManager->pLight[lightIndex];
        Model3D* currentModel = &currentLight->model;
        Mesh3DManager* meshManager = &currentModel->meshManager;

        // Print light properties first
        printf("Volumetric Light %u - Properties: Transparency: %u, Intensity: %u, FallOff: %u\n",
            lightIndex,
            currentLight->properties.transparency,
            currentLight->properties.intensity,
            currentLight->properties.fallOf);

        // Iterate through all meshes in current light's model
        for (uint32_t meshIndex = 0; meshIndex < meshManager->meshesLen; meshIndex++) {
            Mesh3D* currentMesh = &meshManager->pMesh[meshIndex];

            // Iterate through all vertices in current mesh
            for (uint32_t vertexIndex = 0; vertexIndex < currentMesh->verticesLen; vertexIndex++) {
                GEOM_VERTEX vertex = currentMesh->pVertice[vertexIndex];
                printf("Light %u, Mesh %u, Vertex %u - (X,Y,Z):{%f,%f,%f}\n",
                    lightIndex,
                    meshIndex,
                    vertexIndex,
                    vertex.point.x,
                    vertex.point.y,
                    vertex.point.z);
            }
        }
    }
}

void printLightVolume(LightVolume* const light) {
    if (!light) {
        printf("Error: Invalid light pointer\n");
        return;
    }

    // Print light properties
    printf("Light Properties: Transparency: %u, Intensity: %u, FallOff: %u\n",
        light->properties.transparency,
        light->properties.intensity,
        light->properties.fallOf);

    // Get the model and mesh manager
    Model3D* currentModel = &light->model;
    Mesh3DManager* meshManager = &currentModel->meshManager;

    // Iterate through all meshes in the light's model
    for (uint32_t meshIndex = 0; meshIndex < meshManager->meshesLen; meshIndex++) {
        Mesh3D* currentMesh = &meshManager->pMesh[meshIndex];

        // Iterate through all vertices in current mesh
        for (uint32_t vertexIndex = 0; vertexIndex < currentMesh->verticesLen; vertexIndex++) {
            GEOM_VERTEX vertex = currentMesh->pVertice[vertexIndex];
            printf("Mesh %u, Vertex %u - (X,Y,Z):{%f,%f,%f}\n",
                meshIndex,
                vertexIndex,
                vertex.point.x,
                vertex.point.y,
                vertex.point.z);
        }
    }
}

void printScene_(GEOM_OBJECT* objects, uint32_t objectsLen){
    //printf("OBJECTS: %x -> %d\n",objects, objectsLen);
    for(int objIndex = 0; objIndex < objectsLen; objIndex++){
        for(int vertexIndex = 0; vertexIndex < objects[objIndex].dataLen; vertexIndex++)
            printf("(X,Y,Z):{%f,%f,%f} \n", objects[objIndex].data[vertexIndex].point.x,
            objects[objIndex].data[vertexIndex].point.y,
            objects[objIndex].data[vertexIndex].point.z);
    }
};

void fillSceneWithModelData(SCENE* scene, Model3D* pModel, uint32_t modelLen){
    printf("INIT SCENE WITH MODELSLen:%d\n", modelLen);
    uint32_t indexOffsetForModelIndices = 0;
    uint32_t indexOffset = 0;
    uint32_t vertexOffset = 0;
    //printScene(objects, sceneLen);

    for(int objIndex = 0; objIndex < modelLen; objIndex++){
        scene->modelsManager.pModel[objIndex] = pModel[objIndex];
        //printf("SDFUNSOGNOSD\n");
        Model3D* model = &(scene)->modelsManager.pModel[objIndex];
        //apply_offset_to_indices(model->indices.items, model->indices.itemsLen, indexOffsetForModelIndices);
        model->indiceOffset =  indexOffset;
        //indexOffsetForModelIndices += model->meshManager.totalMeshVerticesLen;
        indexOffset += model->indices.itemsLen;
        model->vertexOffset = vertexOffset;
        vertexOffset += model->meshManager.totalMeshVerticesLen;

    }

    //printf("TRAS ITERAR EL BUCLE DE OBJETOS\n");

    (scene)->modelsManager.packedModelVertexLen = getModelVerticesLenFromScene(scene);
    (scene)->modelsManager.pPackedModelVertex = getModelVerticesFromScene(scene);

    (scene)->modelsManager.packedModelIndex.itemsLen = getModelIndicesLenFromScene(scene);
    (scene)->modelsManager.packedModelIndex.items = getModelIndicesFromScene((scene));
    //printf("FINALIZAMOS CREACION ESCENA!\n");
    //printf("SCENELEN : %d", (*scene)->sceneLen);
    //printf("INDICESLEN : %d", (*scene)->totalIndicesLen);
}


void fillModelInstancesIndices(Models3DManager* man) {
    man->instanceOffset.items = malloc(sizeof(*man->instanceOffset.items) * man->modelLen);
    man->instanceOffset.itemsLen = man->modelLen;
    uint32_t instanceOffset = 0;
    for(int itemIdx = 0; itemIdx < man->modelLen; itemIdx++){
        man->instanceOffset.items[itemIdx] = instanceOffset;
        instanceOffset += man->pModel[itemIdx].instanceCount;
    }

}

void fillLightVolumeInstancesIndices(LightManager* lan) {
    lan->instanceOffset.items = malloc(sizeof(*lan->instanceOffset.items) * lan->lightLen);
    lan->instanceOffset.itemsLen = lan->lightLen;
    uint32_t instanceOffset = 0;
    for(int itemIdx = 0; itemIdx < lan->lightLen; itemIdx++){
        lan->instanceOffset.items[itemIdx] = instanceOffset;
        instanceOffset += lan->pLight[itemIdx].model.instanceCount;
    }

}

void fillSceneWithInstancedModelData(Models3DManager* man, Model3DInstance** instanceData, uint32_t totalInstanceDataLen){
    printf("INIT SCENE WITH INSTANCE MODELSLen:%d\n", man->modelLen);

    fillModelInstancesIndices(man);
    man->pModelInstance = malloc(sizeof(*man->pModelInstance) * totalInstanceDataLen);
    //printScene(objects, sceneLen);
    uint32_t modelInstanceIdx = 0;
    man->modelInstanceLen = totalInstanceDataLen;
    for(int modelIdx = 0; modelIdx < man->modelLen; modelIdx++){
        uint32_t instanceLen = man->pModel[modelIdx].instanceCount;
            for(int instanceIdx = 0; instanceIdx < man->pModel[modelIdx].instanceCount; instanceIdx++){
                man->pModelInstance[modelInstanceIdx++] = instanceData[modelIdx][instanceIdx];

            }
    }
}

void fillSceneWithInstancedLightVolumeData(LightManager* lan, LightVolumeInstance** instanceData, uint32_t totalInstanceDataLen){
    printf("INIT SCENE WITH INSTANCE LIGHTLen:%d\n", lan->lightLen);
    fillLightVolumeInstancesIndices(lan);
    lan->pLightInstance = malloc(sizeof(*lan->pLightInstance) * totalInstanceDataLen);
    //printScene(objects, sceneLen);
    uint32_t lightInstanceIdx = 0;
    lan->lightInstanceLen = totalInstanceDataLen;
    for(int lightIdx = 0; lightIdx < lan->lightLen; lightIdx++){
        uint32_t instanceLen = lan->pLight[lightIdx].model.instanceCount;
            for(int instanceIdx = 0; instanceIdx < lan->pLight[lightIdx].model.instanceCount; instanceIdx++){
                lan->pLightInstance[lightInstanceIdx++] = instanceData[lightIdx][instanceIdx];

            }
    }
}
void fillSceneWithLightData(SCENE* scene, LightVolume* pLight, uint32_t volumetricLightsLen){
    printf("INIT SCENE WITH VOLUMETRICLIGHTSLen:%d\n", volumetricLightsLen);
    uint32_t vertexIndexOffsetForModelIndices = 0;
    uint32_t indexOffset = 0;
    uint32_t vertexOffset = 0;
    //printScene(objects, sceneLen);

    for(int objIndex = 0; objIndex < volumetricLightsLen; objIndex++){
        scene->lightManager.pLight[objIndex] = pLight[objIndex];
        printLightVolume(pLight);
        printf("SDFUNSOGNOSD\n");
        LightVolume* light = &(scene)->lightManager.pLight[objIndex];

        //apply_offset_to_indices(light->model.indices.items, light->model.indices.itemsLen, vertexIndexOffsetForModelIndices);
        //vertexIndexOffsetForModelIndices += light->model.meshManager.totalMeshVerticesLen;
        light->model.indiceOffset = indexOffset;
        light->model.vertexOffset = vertexOffset;
        indexOffset += light->model.indices.itemsLen;
        vertexOffset += light->model.meshManager.totalMeshVerticesLen;
    }

    //printf("TRAS ITERAR EL BUCLE DE OBJETOS\n");
    (scene)->lightManager.packedLightLen = getVolumetricLightVerticesLenFromScene(scene);
    (scene)->lightManager.pPackedLight =  getLightVolumesVerticesFromScene(scene);

    (scene)->lightManager.packedLightIndex.itemsLen = getVolumetricLightsIndicesLenFromScene(scene);
    (scene)->lightManager.packedLightIndex.items = getVolumetricLightsIndicesFromScene((scene));

    //printf("FINALIZAMOS CREACION ESCENA!\n");
    //printf("SCENELEN : %d", (*scene)->sceneLen);
    //printf("INDICESLEN : %d", (*scene)->totalIndicesLen);
}
