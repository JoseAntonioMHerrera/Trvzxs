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

    return indicesLen;
}

uint32_t* getModelIndicesFromScene(const SCENE* const scene)
{
    assert(scene->modelsManager.packedModelIndex.itemsLen > 0);

    uint32_t* indices = malloc(sizeof(uint32_t) * scene->modelsManager.packedModelIndex.itemsLen);
    initIndicesScene(indices, scene->modelsManager.packedModelIndex.itemsLen);

    uint32_t indicesStorageIterator = 0;

    // Copy individual array of objects indices to scene indices array
    uint32_t iteratorStorageIndices = 0;

    for (int i = 0; i < scene->modelsManager.modelLen; i++)
    {
        uint32_t currentObjectIndicesLen = scene->modelsManager.pModel[i].indices.itemsLen;
        uint32_t startPositionOffset     = iteratorStorageIndices;

        uint32_t iteratorReadIndices = 0;
        for (iteratorStorageIndices; iteratorStorageIndices < (startPositionOffset + currentObjectIndicesLen);
             iteratorStorageIndices++)
        {
            indices[indicesStorageIterator] = scene->modelsManager.pModel[i].indices.items[iteratorReadIndices];
            indicesStorageIterator++;
            iteratorReadIndices++;
        }
    }
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
        }
    }

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
                pVertex[indicesStorageIterator] =
                    scene->modelsManager.pModel[i].meshManager.pMesh[j].pVertice[indexObjVertex];
                // pVertex[indexObjVertex].point.x,pVertex[indexObjVertex].point.y,pVertex[indexObjVertex].point.z);
                indicesStorageIterator++;
            }
        }
    }
    return pVertex;
}

uint32_t getVolumetricLightsIndicesLenFromScene(const SCENE* const scene)
{
    uint32_t indicesLen = 0;
    for (int i = 0; i < scene->lightManager.lightLen; i++)
    {
        indicesLen += scene->lightManager.pLight[i].model.indices.itemsLen;
    }

    return indicesLen;
}

uint32_t* getVolumetricLightsIndicesFromScene(const SCENE* const scene)
{
    if (scene->lightManager.packedLightIndex.itemsLen > 0)
    {
        return 0;
    }

    uint32_t* indices = malloc(sizeof(uint32_t) * scene->lightManager.packedLightIndex.itemsLen);

    initIndicesScene(indices, scene->lightManager.packedLightIndex.itemsLen);

    uint32_t indicesStorageIterator = 0;
    uint32_t iteratorStorageIndices = 0;
    uint32_t totalSceneObjects      = scene->lightManager.lightLen;

    for (int i = 0; i < scene->lightManager.lightLen; i++)
    {
        uint32_t currentObjectIndicesLen = scene->lightManager.pLight[i].model.indices.itemsLen;

        uint32_t startPositionOffset = iteratorStorageIndices; // flag current offset

        uint32_t iteratorReadIndices = 0;
        for (iteratorStorageIndices; iteratorStorageIndices < (startPositionOffset + currentObjectIndicesLen);
             iteratorStorageIndices++)
        {

            indices[indicesStorageIterator] = scene->lightManager.pLight[i].model.indices.items[iteratorReadIndices];
            indicesStorageIterator++;
            iteratorReadIndices++;
        }
    }

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
    GEOM_VERTEX* pVertex                = malloc(sizeof(GEOM_VERTEX) * scene->lightManager.packedLightLen);
    uint32_t     indicesStorageIterator = 0;

    // Copy individual array of objects indices to scene indices array
    for (int i = 0; i < scene->lightManager.lightLen; i++)
    {
        for (int j = 0; j < scene->lightManager.pLight[i].model.meshManager.meshesLen; j++)
        {
            for (int indexObjVertex = 0;
                 indexObjVertex < scene->lightManager.pLight[i].model.meshManager.pMesh[j].verticesLen;
                 indexObjVertex++)
            {
                pVertex[indicesStorageIterator] =
                    scene->lightManager.pLight[i].model.meshManager.pMesh[j].pVertice[indexObjVertex];
                indicesStorageIterator++;
            }
        }
    }

    return pVertex;
}

/*
    SCENE** scene: pointer reference to init the memory for the scene;
    uint32_t modelsLen: number of GEOM_OBJE

*/
void initScene(SCENE** scene, uint32_t modelsLen, uint32_t lightsLen, uint32_t particleVerticesLen,
               uint32_t particleGroupsLen, uint32_t particleInstancesLen)
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

        (*scene)->particleManager.packedParticleVertexLen = particleVerticesLen;
        (*scene)->particleManager.particleGroupLen        = particleGroupsLen;
        (*scene)->particleManager.pParticleGroup = (ParticleGroup*) malloc(sizeof(ParticleGroup) * particleGroupsLen);
        (*scene)->particleManager.pParticleInstance =
            (ParticleInstance*) malloc(sizeof(ParticleInstance) * particleInstancesLen);
        (*scene)->particleManager.packedParticleInstanceLen = particleInstancesLen;
    }
}

void printSceneModels(const Models3DManager* const modelManager)
{

    // Iterate through all models
    for (uint32_t modelIndex = 0; modelIndex < modelManager->modelLen; modelIndex++)
    {
        Model3D* currentModel = &modelManager->pModel[modelIndex];

        Mesh3DManager* meshManager = &currentModel->meshManager;

        // Iterate through all meshes in current model
        for (uint32_t meshIndex = 0; meshIndex < meshManager->meshesLen; meshIndex++)
        {
            Mesh3D* currentMesh = &meshManager->pMesh[meshIndex];

            if (currentMesh->verticesLen == 0)
            {
                continue;
            }

            // Iterate through all vertices in current mesh
            for (uint32_t vertexIndex = 0; vertexIndex < currentMesh->verticesLen; vertexIndex += 1000)
            {
                GEOM_VERTEX vertex = currentMesh->pVertice[vertexIndex];
            }
        }
    }
}

void printSceneLights(const LightManager* const volManager)
{
    // Iterate through all volumetric lights
    for (uint32_t lightIndex = 0; lightIndex < volManager->lightLen; lightIndex++)
    {
        LightVolume*   currentLight = &volManager->pLight[lightIndex];
        Model3D*       currentModel = &currentLight->model;
        Mesh3DManager* meshManager  = &currentModel->meshManager;

        // Iterate through all meshes in current light's model
        for (uint32_t meshIndex = 0; meshIndex < meshManager->meshesLen; meshIndex++)
        {
            Mesh3D* currentMesh = &meshManager->pMesh[meshIndex];

            // Iterate through all vertices in current mesh
            for (uint32_t vertexIndex = 0; vertexIndex < currentMesh->verticesLen; vertexIndex++)
            {
                GEOM_VERTEX vertex = currentMesh->pVertice[vertexIndex];
            }
        }
    }
}

void printLightVolume(LightVolume* const light)
{
    if (!light)
    {
        return;
    }

    // Print light properties
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
        }
    }
}

void printScene_(GEOM_OBJECT* objects, uint32_t objectsLen) {};

void fillModelInstancesIndices(Models3DManager* man)
{

    man->instanceOffset.items    = malloc(sizeof(*man->instanceOffset.items) * man->modelLen);
    man->instanceOffset.itemsLen = man->modelLen;

    uint32_t instanceOffset = 0;
    for (int itemIdx = 0; itemIdx < man->modelLen; itemIdx++)
    {
        man->instanceOffset.items[itemIdx] = instanceOffset;
        instanceOffset += man->pModel[itemIdx].instanceCount;
    }
}

void fillSceneWithModelData(SCENE* scene, Model3D* pModel, uint32_t modelLen)
{
    uint32_t indexOffsetForModelIndices   = 0;
    uint32_t indexOffset                  = 0;
    uint32_t vertexOffset                 = 0;
    scene->modelsManager.modelInstanceLen = 0;

    for (int objIndex = 0; objIndex < modelLen; objIndex++)
    {
        scene->modelsManager.pModel[objIndex] = pModel[objIndex];

        Model3D* model = &(scene)->modelsManager.pModel[objIndex];

        model->indiceOffset = indexOffset;
        indexOffset += model->indices.itemsLen;

        model->vertexOffset = vertexOffset;
        vertexOffset += model->meshManager.totalMeshVerticesLen;

        scene->modelsManager.modelInstanceLen += model->instanceCount;
    }

    (scene)->modelsManager.packedModelVertexLen = getModelVerticesLenFromScene(scene);
    (scene)->modelsManager.pPackedModelVertex   = getModelVerticesFromScene(scene);

    (scene)->modelsManager.packedModelIndex.itemsLen = getModelIndicesLenFromScene(scene);
    (scene)->modelsManager.packedModelIndex.items    = getModelIndicesFromScene((scene));

    fillModelInstancesIndices(&scene->modelsManager);
}

void fillLightVolumeInstancesIndices(LightManager* lan)
{

    lan->instanceOffset.items    = malloc(sizeof(*lan->instanceOffset.items) * lan->lightLen);
    lan->instanceOffset.itemsLen = lan->lightLen;

    uint32_t instanceOffset = 0;
    for (int itemIdx = 0; itemIdx < lan->lightLen; itemIdx++)
    {
        lan->instanceOffset.items[itemIdx] = instanceOffset;

        instanceOffset += lan->pLight[itemIdx].model.instanceCount;
    }
}
void fillParticleGroupsInstancesIndices(ParticleGroupManager* pgm)
{

    pgm->particleInstanceOffsets.items    = malloc(sizeof(*pgm->particleInstanceOffsets.items) * pgm->particleGroupLen);
    pgm->particleInstanceOffsets.itemsLen = pgm->particleGroupLen;

    uint32_t instanceOffset = 0;
    for (int itemIdx = 0; itemIdx < pgm->particleGroupLen; itemIdx++)
    {
        pgm->particleInstanceOffsets.items[itemIdx] = instanceOffset;
        instanceOffset += pgm->pParticleGroup[itemIdx].particleInstancesLen;
    }
}

void fillSceneWithLightData(SCENE* scene, LightVolume* pLight, uint32_t volumetricLightsLen)
{
    uint32_t vertexIndexOffsetForModelIndices = 0;
    uint32_t indexOffset                      = 0;
    uint32_t vertexOffset                     = 0;
    scene->lightManager.lightInstanceCount    = 0;
    // printScene(objects, sceneLen);

    for (int objIndex = 0; objIndex < volumetricLightsLen; objIndex++)
    {
        scene->lightManager.pLight[objIndex] = pLight[objIndex];
        printLightVolume(pLight);
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

    (scene)->lightManager.packedLightLen = getVolumetricLightVerticesLenFromScene(scene);
    (scene)->lightManager.pPackedLight   = getLightVolumesVerticesFromScene(scene);

    (scene)->lightManager.packedLightIndex.itemsLen = getVolumetricLightsIndicesLenFromScene(scene);
    (scene)->lightManager.packedLightIndex.items    = getVolumetricLightsIndicesFromScene((scene));
    fillLightVolumeInstancesIndices(&scene->lightManager);
}

void fillSceneWithParticleData(SCENE* scene, ParticleGroup* particleGroups, uint32_t particleGroupsLen,
                               TrzVertex* particleVertices, uint32_t particleVerticesCount,
                               uint32_t* particleVertexIndices, uint32_t particleVertexIndicesLen)
{

    uint32_t vertexIndexOffsetForModelIndices = 0;
    uint32_t indexOffset                      = 0;
    uint32_t vertexOffset                     = 0;
    uint32_t packedParticleInstancesOffset    = 0;
    scene->particleManager.particleGroupLen   = particleGroupsLen;

    for (int objIndex = 0; objIndex < particleGroupsLen; objIndex++)
    {
        scene->particleManager.pParticleGroup[objIndex] = particleGroups[objIndex];

        for (int particleInstanceIndex = 0;
             particleInstanceIndex < scene->particleManager.pParticleGroup[objIndex].particleInstancesLen;
             particleInstanceIndex++)
        {
            glm_vec4_copy(
                scene->particleManager.pParticleGroup[objIndex].pParticleInstances[particleInstanceIndex].position,
                scene->particleManager.pParticleInstance[packedParticleInstancesOffset].position);

            glm_vec4_copy(
                scene->particleManager.pParticleGroup[objIndex].pParticleInstances[particleInstanceIndex].velocity,
                scene->particleManager.pParticleInstance[packedParticleInstancesOffset].velocity);

            glm_vec4_copy(
                scene->particleManager.pParticleGroup[objIndex].pParticleInstances[particleInstanceIndex].color,
                scene->particleManager.pParticleInstance[packedParticleInstancesOffset].color);

            packedParticleInstancesOffset++;
        }
    }

    scene->particleManager.pPackedParticleVertex     = particleVertices;
    scene->particleManager.packedParticleVertexLen   = particleVerticesCount;
    scene->particleManager.pPackedParticleIndices    = particleVertexIndices;
    scene->particleManager.packedParticleIndicesLen  = particleVertexIndicesLen;
    scene->particleManager.packedParticleInstanceLen = packedParticleInstancesOffset;

    fillParticleGroupsInstancesIndices(&scene->particleManager);
}
