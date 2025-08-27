#include "scene.h"
#include "light.h"
#include "vertex.h"
#include <math.h>
#include <stdlib.h>
void apply_offset_to_indices(TRVZXS_SIZE* indices, size_t num_indices, TRVZXS_SIZE offset)
{
    for (size_t i = 0; i < num_indices; i++)
    {
        indices[i] += offset;
    }
}

void initIndicesScene(uint32_t* indices, uint32_t indicesLen)
{
    for (int i = 0; i < indicesLen; i++)
    {
        indices[i] = 0;
    }
}

uint32_t getModelIndicesLenFromScene(const SCENE* const scene)
{
    uint32_t indicesLen = 0;
    for (int i = 0; i < scene->modelsManager.modelLen; i++)
    {
        indicesLen += scene->modelsManager.pModel[i].indices.itemsLen;
    }

    printf("getModelINdicesLenFromScene", indicesLen);
    return indicesLen;
}

uint32_t* getModelIndicesFromScene(const SCENE* const scene)
{
    printf("DEBUG: Starting getModelIndicesFromScene\n");
    assert(scene->modelsManager.packedModelIndex.itemsLen > 0);
    printf("DEBUG: packedModelIndex.itemsLen = %u\n", scene->modelsManager.packedModelIndex.itemsLen);

    uint32_t* indices = malloc(sizeof(uint32_t) * scene->modelsManager.packedModelIndex.itemsLen);
    printf("DEBUG: Allocated memory for %u indices\n", scene->modelsManager.packedModelIndex.itemsLen);

    initIndicesScene(indices, scene->modelsManager.packedModelIndex.itemsLen);
    printf("DEBUG: Initialized indices array\n");

    uint32_t indicesStorageIterator = 0;

    // Copy individual array of objects indices to scene indices array
    uint32_t iteratorStorageIndices = 0;
    uint32_t totalSceneObjects      = scene->modelsManager.modelLen;
    printf("DEBUG: Total scene objects = %u\n", totalSceneObjects);

    for (int i = 0; i < scene->modelsManager.modelLen; i++)
    {
        uint32_t currentObjectIndicesLen = scene->modelsManager.pModel[i].indices.itemsLen;
        printf("DEBUG: Processing model %d with %u indices\n", i, currentObjectIndicesLen);

        uint32_t startPositionOffset = iteratorStorageIndices; // flag current offset
        printf("DEBUG: Start position offset = %u\n", startPositionOffset);

        uint32_t iteratorReadIndices = 0;
        for (iteratorStorageIndices; iteratorStorageIndices < (startPositionOffset + currentObjectIndicesLen);
             iteratorStorageIndices++)
        {
            // printf("ITERATOR:%d\n", indicesStorageIterator);
            /*printf("DEBUG: Copying index %u from model %d position %u to global position %u\n",
                   scene->modelsManager.pModel[i].indices.items[iteratorReadIndices], i, iteratorReadIndices,
                   indicesStorageIterator);*/

            indices[indicesStorageIterator] = scene->modelsManager.pModel[i].indices.items[iteratorReadIndices];
            indicesStorageIterator++;
            iteratorReadIndices++;
        }
        printf("DEBUG: Finished processing model %d, indicesStorageIterator = %u\n", i, indicesStorageIterator);
    }

    printf("DEBUG: Completed getModelIndicesFromScene, returning indices array\n");
    return indices;
}

uint32_t getModelVerticesLenFromScene(const SCENE* const scene)
{
    uint32_t dataLen = 0;
    for (int i = 0; i < scene->modelsManager.modelLen; i++)
    {
        for (int j = 0; j < scene->modelsManager.pModel[i].meshManager.meshesLen; j++)
        {
            dataLen += scene->modelsManager.pModel[i].meshManager.pMesh[j].verticesLen;
            printf("AÑADIMOS VERTICES LEN EN GETMODELVERTICESLENfromScene\n");
        }
    }

    printf("RETORNAMOS %d DATA LENNNNNNNNNNNNNN\n\n\n\n\n", dataLen);
    return dataLen;
}

GEOM_VERTEX* getModelVerticesFromScene(const SCENE* const scene)
{
    assert(scene->modelsManager.modelLen > 0);
    GEOM_VERTEX* pVertex                = malloc(sizeof(GEOM_VERTEX) * scene->modelsManager.packedModelVertexLen);
    uint32_t     indicesStorageIterator = 0;

    // Copy individual array of objects indices to scene indices array
    for (int i = 0; i < scene->modelsManager.modelLen; i++)
    {
        for (int j = 0; j < scene->modelsManager.pModel[i].meshManager.meshesLen; j++)
        {
            for (int indexObjVertex = 0;
                 indexObjVertex < scene->modelsManager.pModel[i].meshManager.pMesh[j].verticesLen; indexObjVertex++)
            {
                // printf("ITERATOR:%d\n", indicesStorageIterator);
                pVertex[indicesStorageIterator] =
                    scene->modelsManager.pModel[i].meshManager.pMesh[j].pVertice[indexObjVertex];
                // printf("ITERATOR:%d %d %d\n",
                // pVertex[indexObjVertex].point.x,pVertex[indexObjVertex].point.y,pVertex[indexObjVertex].point.z);
                indicesStorageIterator++;
            }
        }
    }
    printf("FINALIZAMOS getModelVerticesFromScene\n");
    return pVertex;
}

uint32_t getVolumetricLightsIndicesLenFromScene(const SCENE* const scene)
{
    uint32_t indicesLen = 0;
    for (int i = 0; i < scene->lightManager.lightLen; i++)
    {
        indicesLen += scene->lightManager.pLight[i].model.indices.itemsLen;
    }

    // printf("RETORNAMOS %d INDICES LEN\n", indicesLen);
    return indicesLen;
}

uint32_t* getVolumetricLightsIndicesFromScene(const SCENE* const scene)
{
    printf("DEBUG: Starting getVolumetricLightsIndicesFromScene\n");
    if (scene->lightManager.packedLightIndex.itemsLen > 0)
    {
        return 0;
    }
    printf("DEBUG: packedLightIndex.itemsLen = %u\n", scene->lightManager.packedLightIndex.itemsLen);

    uint32_t* indices = malloc(sizeof(uint32_t) * scene->lightManager.packedLightIndex.itemsLen);
    printf("DEBUG: Allocated memory for %u indices\n", scene->lightManager.packedLightIndex.itemsLen);

    initIndicesScene(indices, scene->lightManager.packedLightIndex.itemsLen);
    printf("DEBUG: Initialized indices array\n");

    uint32_t indicesStorageIterator = 0;
    uint32_t iteratorStorageIndices = 0;
    uint32_t totalSceneObjects      = scene->lightManager.lightLen;
    printf("DEBUG: Total scene light objects = %u\n", totalSceneObjects);

    for (int i = 0; i < scene->lightManager.lightLen; i++)
    {
        uint32_t currentObjectIndicesLen = scene->lightManager.pLight[i].model.indices.itemsLen;
        printf("DEBUG: Processing light %d with %u indices\n", i, currentObjectIndicesLen);

        uint32_t startPositionOffset = iteratorStorageIndices; // flag current offset
        printf("DEBUG: Start position offset = %u\n", startPositionOffset);

        uint32_t iteratorReadIndices = 0;
        for (iteratorStorageIndices; iteratorStorageIndices < (startPositionOffset + currentObjectIndicesLen);
             iteratorStorageIndices++)
        {
            // printf("ITERATOR:%d\n", indicesStorageIterator);
            /*printf("DEBUG: Copying index %u from light %d position %u to global position %u with value %u\n",
                   scene->modelsManager.pModel[i].indices.items[iteratorReadIndices], i, iteratorReadIndices,
                   indicesStorageIterator, scene->lightManager.pLight[i].model.indices.items[iteratorReadIndices]);*/

            indices[indicesStorageIterator] = scene->lightManager.pLight[i].model.indices.items[iteratorReadIndices];
            indicesStorageIterator++;
            iteratorReadIndices++;
        }
        printf("DEBUG: Finished processing light %d, indicesStorageIterator = %u\n", i, indicesStorageIterator);
    }

    printf("DEBUG: Completed getVolumetricLightsIndicesFromScene, returning indices array\n");

    return indices;
}

uint32_t getVolumetricLightVerticesLenFromScene(const SCENE* const scene)
{
    uint32_t dataLen = 0;
    for (int i = 0; i < scene->lightManager.lightLen; i++)
    {
        for (int j = 0; j < scene->lightManager.pLight[i].model.meshManager.meshesLen; j++)
        {
            dataLen += scene->lightManager.pLight[i].model.meshManager.pMesh[j].verticesLen;
        }
    }
    return dataLen;
}

GEOM_VERTEX* getLightVolumesVerticesFromScene(const SCENE* const scene)
{
    assert(scene->lightManager.packedLightLen > 0);
    printf("FLAG 1 getVolumetricLightsVerticesFromScene %d\n",
           scene->lightManager.pLight[0].model.meshManager.pMesh[0].verticesLen);
    printf("FLAG 1 lightlen %d\n", scene->lightManager.lightLen);
    printf("FLAG 1 meshlen %d\n", scene->lightManager.pLight[0].model.meshManager.meshesLen);
    GEOM_VERTEX* pVertex                = malloc(sizeof(GEOM_VERTEX) * scene->lightManager.packedLightLen);
    uint32_t     indicesStorageIterator = 0;
    printf("FLAG 2 getVolumetricLightsVerticesFromScene\n");

    // Copy individual array of objects indices to scene indices array
    for (int i = 0; i < scene->lightManager.lightLen; i++)
    {
        printf("FLAG BAGGGGGGGGG\n");
        for (int j = 0; j < scene->lightManager.pLight[i].model.meshManager.meshesLen; j++)
        {
            for (int indexObjVertex = 0;
                 indexObjVertex < scene->lightManager.pLight[i].model.meshManager.pMesh[j].verticesLen;
                 indexObjVertex++)
            {
                printf("FLAG 2.5 getVolumetricLightsVerticesFromScene\n");
                // printf("ITERATOR:%d\n", indicesStorageIterator);
                pVertex[indicesStorageIterator] =
                    scene->lightManager.pLight[i].model.meshManager.pMesh[j].pVertice[indexObjVertex];
                printf("%f, %f, %f", pVertex[indicesStorageIterator].point.x, pVertex[indicesStorageIterator].point.y,
                       pVertex[indicesStorageIterator].point.z);
                // printf("ITERATOR:%d %d %d\n",
                // pVertex[indexObjVertex].point.x,pVertex[indexObjVertex].point.y,pVertex[indexObjVertex].point.z);
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
void initScene(SCENE** scene, uint32_t modelsLen, uint32_t lightsLen, uint32_t particleVerticesLen,
               uint32_t particleGroupsLen)
{
    (*scene) = (SCENE*) malloc(sizeof(SCENE));
    if (modelsLen <= 0)
    {
        (*scene)->modelsManager.modelLen = 0;
    }
    else
    {
        (*scene)->modelsManager.pModel   = (Model3D*) malloc(sizeof(Model3D) * modelsLen);
        (*scene)->modelsManager.modelLen = modelsLen;
    }

    if (lightsLen <= 0)
    {
        (*scene)->lightManager.lightLen = 0;
    }
    else
    {
        (*scene)->lightManager.pLight   = (LightVolume*) malloc(sizeof(LightVolume) * lightsLen);
        (*scene)->lightManager.lightLen = lightsLen;
    }

    if (particleGroupsLen <= 0)
    {
        (*scene)->particleManager.particleGroupLen = 0;
    }
    else
    {
        printf("WE INIT THAT SHIT\n");
        (*scene)->particleManager.packedParticleVertexLen = particleVerticesLen;
        (*scene)->particleManager.particleGroupLen        = particleGroupsLen;
        (*scene)->particleManager.pParticleGroup = (ParticleGroup*) malloc(sizeof(ParticleGroup) * particleGroupsLen);
    }
}

void printSceneModels(const Models3DManager* const modelManager)
{
    printf("=== PRINTING SCENE MODELS ===\n");
    printf("Total models in manager: %u\n", modelManager->modelLen);
    printf("Models manager address: %p\n", modelManager);
    printf("pModel array address: %p\n", modelManager->pModel);

    // Iterate through all models
    for (uint32_t modelIndex = 0; modelIndex < modelManager->modelLen; modelIndex++)
    {
        printf("Processing model %u/%u\n", modelIndex, modelManager->modelLen - 1);
        Model3D* currentModel = &modelManager->pModel[modelIndex];
        printf("  Current model address: %p\n", currentModel);

        Mesh3DManager* meshManager = &currentModel->meshManager;
        printf("  Mesh manager address: %p\n", meshManager);
        printf("  Meshes in model %u: %u\n", modelIndex, meshManager->meshesLen);

        // Iterate through all meshes in current model
        for (uint32_t meshIndex = 0; meshIndex < meshManager->meshesLen; meshIndex++)
        {
            printf("  Processing mesh %u/%u in model %u\n", meshIndex, meshManager->meshesLen - 1, modelIndex);
            Mesh3D* currentMesh = &meshManager->pMesh[meshIndex];
            printf("    Current mesh address: %p\n", currentMesh);
            printf("    Vertices in mesh %u: %u\n", meshIndex, currentMesh->verticesLen);
            printf("    Vertices array address: %p\n", currentMesh->pVertice);

            if (currentMesh->verticesLen == 0)
            {
                printf("    WARNING: Mesh %u has 0 vertices!\n", meshIndex);
                continue;
            }

            printf("    Sampling every 1000th vertex in mesh %u:\n", meshIndex);

            // Iterate through all vertices in current mesh
            for (uint32_t vertexIndex = 0; vertexIndex < currentMesh->verticesLen; vertexIndex += 1000)
            {
                printf("    Accessing vertex %u/%u\n", vertexIndex, currentMesh->verticesLen - 1);
                GEOM_VERTEX vertex = currentMesh->pVertice[vertexIndex];
                printf("Model %u, Mesh %u, Vertex %u - (X,Y,Z):{%f,%f,%f}\n", modelIndex, meshIndex, vertexIndex,
                       vertex.point.x, vertex.point.y, vertex.point.z);
            }

            printf("    Completed sampling mesh %u\n", meshIndex);
        }
        printf("  Completed processing model %u\n", modelIndex);
    }

    printf("=== SCENE MODELS PRINTING COMPLETE ===\n");
}

void printSceneLights(const LightManager* const volManager)
{
    // Iterate through all volumetric lights
    for (uint32_t lightIndex = 0; lightIndex < volManager->lightLen; lightIndex++)
    {
        LightVolume*   currentLight = &volManager->pLight[lightIndex];
        Model3D*       currentModel = &currentLight->model;
        Mesh3DManager* meshManager  = &currentModel->meshManager;

        // Print light properties first
        printf("Volumetric Light %u - Properties: Transparency: %u, Intensity: %u, FallOff: %u\n", lightIndex,
               currentLight->properties.transparency, currentLight->properties.intensity,
               currentLight->properties.fallOf);

        // Iterate through all meshes in current light's model
        for (uint32_t meshIndex = 0; meshIndex < meshManager->meshesLen; meshIndex++)
        {
            Mesh3D* currentMesh = &meshManager->pMesh[meshIndex];

            // Iterate through all vertices in current mesh
            for (uint32_t vertexIndex = 0; vertexIndex < currentMesh->verticesLen; vertexIndex++)
            {
                GEOM_VERTEX vertex = currentMesh->pVertice[vertexIndex];
                printf("Light %u, Mesh %u, Vertex %u - (X,Y,Z):{%f,%f,%f}\n", lightIndex, meshIndex, vertexIndex,
                       vertex.point.x, vertex.point.y, vertex.point.z);
            }
        }
    }
}

void printLightVolume(LightVolume* const light)
{
    if (!light)
    {
        printf("Error: Invalid light pointer\n");
        return;
    }

    // Print light properties
    printf("Light Properties: Transparency: %u, Intensity: %u, FallOff: %u\n", light->properties.transparency,
           light->properties.intensity, light->properties.fallOf);

    // Get the model and mesh manager
    Model3D*       currentModel = &light->model;
    Mesh3DManager* meshManager  = &currentModel->meshManager;

    // Iterate through all meshes in the light's model
    for (uint32_t meshIndex = 0; meshIndex < meshManager->meshesLen; meshIndex++)
    {
        Mesh3D* currentMesh = &meshManager->pMesh[meshIndex];

        // Iterate through all vertices in current mesh
        for (uint32_t vertexIndex = 0; vertexIndex < currentMesh->verticesLen; vertexIndex++)
        {
            GEOM_VERTEX vertex = currentMesh->pVertice[vertexIndex];
            printf("Mesh %u, Vertex %u - (X,Y,Z):{%f,%f,%f}\n", meshIndex, vertexIndex, vertex.point.x, vertex.point.y,
                   vertex.point.z);
        }
    }
}

void printScene_(GEOM_OBJECT* objects, uint32_t objectsLen)
{
    // printf("OBJECTS: %x -> %d\n",objects, objectsLen);
    for (int objIndex = 0; objIndex < objectsLen; objIndex++)
    {
        for (int vertexIndex = 0; vertexIndex < objects[objIndex].dataLen; vertexIndex++)
            printf("(X,Y,Z):{%f,%f,%f} \n", objects[objIndex].data[vertexIndex].point.x,
                   objects[objIndex].data[vertexIndex].point.y, objects[objIndex].data[vertexIndex].point.z);
    }
};

void fillModelInstancesIndices(Models3DManager* man)
{
    printf("DEBUG: fillModelInstancesIndices - Starting with %d models\n", man->modelLen);

    man->instanceOffset.items    = malloc(sizeof(*man->instanceOffset.items) * man->modelLen);
    man->instanceOffset.itemsLen = man->modelLen;

    printf("DEBUG: Allocated instanceOffset array for %d items\n", man->modelLen);

    uint32_t instanceOffset = 0;
    for (int itemIdx = 0; itemIdx < man->modelLen; itemIdx++)
    {
        man->instanceOffset.items[itemIdx] = instanceOffset;
        printf("DEBUG: Model[%d]: instanceOffset=%u, instanceCount=%u\n", itemIdx, instanceOffset,
               man->pModel[itemIdx].instanceCount);
        instanceOffset += man->pModel[itemIdx].instanceCount;
    }

    printf("DEBUG: fillModelInstancesIndices - Completed, total instances: %u\n", instanceOffset);
}

void fillSceneWithModelData(SCENE* scene, Model3D* pModel, uint32_t modelLen)
{
    printf("INIT SCENE WITH MODELSLen:%d\n", modelLen);
    uint32_t indexOffsetForModelIndices   = 0;
    uint32_t indexOffset                  = 0;
    uint32_t vertexOffset                 = 0;
    scene->modelsManager.modelInstanceLen = 0;
    // printScene(objects, sceneLen);

    for (int objIndex = 0; objIndex < modelLen; objIndex++)
    {
        printf("=== PROCESSING MODEL %d/%d ===\n", objIndex, modelLen - 1);
        printf("loop %d fillSceneWithModels\n", objIndex);

        // Debug before assignment
        printf("Before assignment - pModel[%d] address: %p\n", objIndex, &pModel[objIndex]);

        scene->modelsManager.pModel[objIndex] = pModel[objIndex];

        // Debug after assignment
        printf("After assignment - scene->modelsManager.pModel[%d] address: %p\n", objIndex,
               &scene->modelsManager.pModel[objIndex]);

        Model3D* model = &(scene)->modelsManager.pModel[objIndex];
        printf("Model pointer: %p\n", model);

        // Debug model properties before offset calculations
        printf("Model %d properties BEFORE offset calculation:\n", objIndex);
        printf("  - indices.itemsLen: %d\n", model->indices.itemsLen);
        printf("  - meshManager.totalMeshVerticesLen: %d\n", model->meshManager.totalMeshVerticesLen);
        printf("  - Current indexOffset: %d\n", indexOffset);
        printf("  - Current vertexOffset: %d\n", vertexOffset);
        printf("  - Instance Count: %d\n", model->instanceCount);
        // Set offsets with debug
        printf("Setting indiceOffset to: %d\n", indexOffset);
        model->indiceOffset = indexOffset;

        printf("Incrementing indexOffset by %d (from %d to %d)\n", model->indices.itemsLen, indexOffset,
               indexOffset + model->indices.itemsLen);
        indexOffset += model->indices.itemsLen;

        printf("Setting vertexOffset to: %d\n", vertexOffset);
        model->vertexOffset = vertexOffset;

        printf("Incrementing vertexOffset by %d (from %d to %d)\n", model->meshManager.totalMeshVerticesLen,
               vertexOffset, vertexOffset + model->meshManager.totalMeshVerticesLen);
        vertexOffset += model->meshManager.totalMeshVerticesLen;

        // Debug final model state
        printf("Model %d properties AFTER offset calculation:\n", objIndex);
        printf("  - indiceOffset: %d\n", model->indiceOffset);
        printf("  - vertexOffset: %d\n", model->vertexOffset);
        printf("  - Running totals - indexOffset: %d, vertexOffset: %d\n", indexOffset, vertexOffset);

        // Validate offsets make sense
        if (model->indiceOffset < 0 || model->vertexOffset < 0)
        {
            printf("ERROR: Negative offset detected for model %d!\n", objIndex);
        }
        scene->modelsManager.modelInstanceLen += model->instanceCount;
        printf("=== MODEL %d PROCESSING COMPLETE ===\n\n", objIndex);
    }

    printf("FINAL TOTALS: indexOffset=%d, vertexOffset=%d\n instanceCount=%d\n", indexOffset, vertexOffset,
           scene->modelsManager.modelInstanceLen);

    // printf("TRAS ITERAR EL BUCLE DE OBJETOS\n");

    (scene)->modelsManager.packedModelVertexLen = getModelVerticesLenFromScene(scene);
    (scene)->modelsManager.pPackedModelVertex   = getModelVerticesFromScene(scene);

    (scene)->modelsManager.packedModelIndex.itemsLen = getModelIndicesLenFromScene(scene);
    (scene)->modelsManager.packedModelIndex.items    = getModelIndicesFromScene((scene));

    fillModelInstancesIndices(&scene->modelsManager);
    // printf("FINALIZAMOS CREACION ESCENA!\n");
    // printf("SCENELEN : %d", (*scene)->sceneLen);
    // printf("INDICESLEN : %d", (*scene)->totalIndicesLen);
}

void fillLightVolumeInstancesIndices(LightManager* lan)
{
    printf("DEBUG: fillLightVolumeInstancesIndices - Starting with %d lights\n", lan->lightLen);

    lan->instanceOffset.items    = malloc(sizeof(*lan->instanceOffset.items) * lan->lightLen);
    lan->instanceOffset.itemsLen = lan->lightLen;

    printf("DEBUG: Allocated instanceOffset array for %d light items\n", lan->lightLen);

    uint32_t instanceOffset = 0;
    for (int itemIdx = 0; itemIdx < lan->lightLen; itemIdx++)
    {
        lan->instanceOffset.items[itemIdx] = instanceOffset;
        printf("DEBUG: Light[%d]: instanceOffset=%u, instanceCount=%u\n", itemIdx, instanceOffset,
               lan->pLight[itemIdx].model.instanceCount);
        instanceOffset += lan->pLight[itemIdx].model.instanceCount;
    }

    printf("DEBUG: fillLightVolumeInstancesIndices - Completed, total light instances: %u\n", instanceOffset);
}
void fillParticleGroupsInstancesIndices(ParticleGroupManager* pgm)
{
    printf("DEBUG: fillParticleGroupsInstancesIndices - Starting with %d particles\n", pgm->particlesInstancesTotalLen);

    pgm->particleInstanceOffsets.items    = malloc(sizeof(*pgm->particleInstanceOffsets.items) * pgm->particleGroupLen);
    pgm->particleInstanceOffsets.itemsLen = pgm->particleGroupLen;

    printf("DEBUG: Allocated instanceOffset array for %d particle items\n", pgm->particleGroupLen);

    uint32_t instanceOffset = 0;
    for (int itemIdx = 0; itemIdx < pgm->particleGroupLen; itemIdx++)
    {
        pgm->particleInstanceOffsets.items[itemIdx] = instanceOffset;
        printf("DEBUG: Particle[%d]: instanceOffset=%u, instanceCount=%u\n", itemIdx, instanceOffset,
               pgm->pParticleGroup[itemIdx].particleInstancesLen);
        instanceOffset += pgm->pParticleGroup[itemIdx].particleInstancesLen;
    }

    printf("DEBUG: fillParticleGroupsInstancesIndices - Completed, total particle instances: %u\n", instanceOffset);
}

void fillSceneWithLightData(SCENE* scene, LightVolume* pLight, uint32_t volumetricLightsLen)
{
    printf("INIT SCENE WITH VOLUMETRICLIGHTSLen:%d\n", volumetricLightsLen);
    uint32_t vertexIndexOffsetForModelIndices = 0;
    uint32_t indexOffset                      = 0;
    uint32_t vertexOffset                     = 0;
    scene->lightManager.lightInstanceCount    = 0;
    // printScene(objects, sceneLen);

    for (int objIndex = 0; objIndex < volumetricLightsLen; objIndex++)
    {
        scene->lightManager.pLight[objIndex] = pLight[objIndex];
        printLightVolume(pLight);
        printf("SDFUNSOGNOSD\n");
        LightVolume* light = &(scene)->lightManager.pLight[objIndex];

        // apply_offset_to_indices(light->model.indices.items, light->model.indices.itemsLen,
        // vertexIndexOffsetForModelIndices); vertexIndexOffsetForModelIndices +=
        // light->model.meshManager.totalMeshVerticesLen;
        light->model.indiceOffset = indexOffset;
        light->model.vertexOffset = vertexOffset;
        indexOffset += light->model.indices.itemsLen;
        vertexOffset += light->model.meshManager.totalMeshVerticesLen;
        scene->lightManager.lightInstanceCount += light->lightVolumeInstanceCount;
    }
    printf("FINAL TOTALS: indexOffset=%d, vertexOffset=%d\n instanceCount=%d\n", indexOffset, vertexOffset,
           scene->lightManager.lightInstanceCount);

    // printf("TRAS ITERAR EL BUCLE DE OBJETOS\n");
    (scene)->lightManager.packedLightLen = getVolumetricLightVerticesLenFromScene(scene);
    (scene)->lightManager.pPackedLight   = getLightVolumesVerticesFromScene(scene);

    (scene)->lightManager.packedLightIndex.itemsLen = getVolumetricLightsIndicesLenFromScene(scene);
    (scene)->lightManager.packedLightIndex.items    = getVolumetricLightsIndicesFromScene((scene));
    fillLightVolumeInstancesIndices(&scene->lightManager);
    // printf("FINALIZAMOS CREACION ESCENA!\n");
    // printf("SCENELEN : %d", (*scene)->sceneLen);
    // printf("INDICESLEN : %d", (*scene)->totalIndicesLen);
}

void fillSceneWithParticleData(SCENE* scene, ParticleGroup* particleGroups, uint32_t particleGroupsLen,
                               GEOM_VERTEX* particleVertices, uint32_t particleVerticesCount,
                               uint32_t* particleVertexIndices, uint32_t particleVertexIndicesLen)
{
    scene->particleManager.particleGroupLen = particleGroupsLen;

    printf("INIT SCENE WITH ParticlesGroupsLen:%d\n", particleGroupsLen);
    uint32_t vertexIndexOffsetForModelIndices         = 0;
    uint32_t indexOffset                              = 0;
    uint32_t vertexOffset                             = 0;
    scene->particleManager.particlesInstancesTotalLen = 0;
    // printScene(objects, sceneLen);

    for (int objIndex = 0; objIndex < particleGroupsLen; objIndex++)
    {
        scene->particleManager.pParticleGroup[objIndex] = particleGroups[objIndex];

        printf("SDFUNSOGNOSD\n");
        ParticleGroup* particleGroup = &(scene)->particleManager.pParticleGroup[objIndex];

        // apply_offset_to_indices(light->model.indices.items, light->model.indices.itemsLen,
        // vertexIndexOffsetForModelIndices); vertexIndexOffsetForModelIndices +=
        // light->model.meshManager.totalMeshVerticesLen;
        scene->particleManager.particlesInstancesTotalLen += particleGroup->particleInstancesLen;
    }
    printf("FINAL TOTALS: indexOffset=%d, vertexOffset=%d\n instanceCount=%d\n", indexOffset, vertexOffset,
           scene->particleManager.particlesInstancesTotalLen);

    scene->particleManager.pPackedParticleVertex    = particleVertices;
    scene->particleManager.packedParticleVertexLen  = particleVerticesCount;
    scene->particleManager.pPackedParticleIndices   = particleVertexIndices;
    scene->particleManager.packedParticleIndicesLen = particleVertexIndicesLen;
    fillParticleGroupsInstancesIndices(&scene->particleManager);
    // printf("FINALIZAMOS CREACION ESCENA!\n");
    // printf("SCENELEN : %d", (*scene)->sceneLen);
    // printf("INDICESLEN : %d", (*scene)->totalIndicesLen);
}
