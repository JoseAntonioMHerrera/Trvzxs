#include "scene.h"
#include "light.h"
#include "vertex.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
        man->instanceOffset.items[itemIdx]   = instanceOffset;
        man->pModel[itemIdx].denseBaseOffset = instanceOffset;
        instanceOffset += man->pModel[itemIdx].instanceCount;
    }

    // Allocate single dense array and consolidate per-model temporary allocations
    man->pDenseModelInstances = (Model3DInstance*) calloc(man->modelInstanceCapacity, sizeof(Model3DInstance));

    for (uint32_t m = 0; m < man->modelLen; m++)
    {
        Model3D* model         = &man->pModel[m];
        uint32_t base          = model->denseBaseOffset;
        if (model->pStagingInstances)
        {
            memcpy(&man->pDenseModelInstances[base], model->pStagingInstances,
                   model->instanceCount * sizeof(Model3DInstance));
            free(model->pStagingInstances);
            model->pStagingInstances = NULL;
        }
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

        // Initialize lifecycle fields for runtime disable/enable/destroy
        model->alive             = 1;
        model->liveInstanceCount = 0;

        model->indiceOffset = indexOffset;
        indexOffset += model->indices.itemsLen;

        model->vertexOffset = vertexOffset;
        vertexOffset += model->meshManager.totalMeshVerticesLen;

        scene->modelsManager.modelInstanceLen += model->instanceCount;
    }

    // Immutable capacity for buffer creation and descriptor range sizing
    scene->modelsManager.modelInstanceCapacity = scene->modelsManager.modelInstanceLen;

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

        // Initialize lifecycle fields
        light->alive              = LIGHT_ALIVE;
        light->liveInstanceCount  = 0;
        for (uint32_t i = 0; i < light->lightVolumeInstanceCount; i++)
        {
            light->pLightVolumeInstance[i].alive = LIGHT_INSTANCE_ALIVE;
        }

        // apply_offset_to_indices(light->model.indices.items, light->model.indices.itemsLen,
        // vertexIndexOffsetForModelIndices); vertexIndexOffsetForModelIndices +=
        // light->model.meshManager.totalMeshVerticesLen;
        light->model.indiceOffset = indexOffset;
        light->model.vertexOffset = vertexOffset;
        indexOffset += light->model.indices.itemsLen;
        vertexOffset += light->model.meshManager.totalMeshVerticesLen;
        scene->lightManager.lightInstanceCount += light->lightVolumeInstanceCount;
    }

    // Immutable capacity for buffer creation and descriptor range sizing
    scene->lightManager.lightInstanceCapacity = scene->lightManager.lightInstanceCount;

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

// ---------------------------------------------------------------------------
// Runtime model lifecycle — instance-level toggle
// ---------------------------------------------------------------------------

void disableInstance(Models3DManager* man, uint32_t modelIndex, uint32_t instanceIndex)
{
    if (modelIndex >= man->modelLen)
    {
        fprintf(stderr, "[disableInstance] modelIndex %u out of range (modelLen=%u)\n", modelIndex, man->modelLen);
        return;
    }

    Model3D* model = &man->pModel[modelIndex];

    if (!model->alive)
    {
        fprintf(stderr, "[disableInstance] model %u is permanently destroyed\n", modelIndex);
        return;
    }

    if (instanceIndex >= model->instanceCount)
    {
        fprintf(stderr, "[disableInstance] instanceIndex %u out of range (instanceCount=%u)\n", instanceIndex,
                model->instanceCount);
        return;
    }

    Model3DInstance* inst = &man->pDenseModelInstances[model->denseBaseOffset + instanceIndex];

    if (inst->alive != 1)
    {
        return;
    }

    inst->alive = 0;
}

void enableInstance(Models3DManager* man, uint32_t modelIndex, uint32_t instanceIndex)
{
    if (modelIndex >= man->modelLen)
    {
        fprintf(stderr, "[enableInstance] modelIndex %u out of range (modelLen=%u)\n", modelIndex, man->modelLen);
        return;
    }

    Model3D* model = &man->pModel[modelIndex];

    if (!model->alive)
    {
        fprintf(stderr, "[enableInstance] model %u is permanently destroyed, cannot re-enable\n", modelIndex);
        return;
    }

    if (instanceIndex >= model->instanceCount)
    {
        fprintf(stderr, "[enableInstance] instanceIndex %u out of range (instanceCount=%u)\n", instanceIndex,
                model->instanceCount);
        return;
    }

    Model3DInstance* inst = &man->pDenseModelInstances[model->denseBaseOffset + instanceIndex];

    if (inst->alive == 2)
    {
        fprintf(stderr, "[enableInstance] instance %u of model %u is permanently destroyed, cannot re-enable\n",
                instanceIndex, modelIndex);
        return;
    }

    inst->alive = 1;
}

// ---------------------------------------------------------------------------
// Runtime model lifecycle — model-level toggle
// ---------------------------------------------------------------------------

void disableModel(Models3DManager* man, uint32_t modelIndex)
{
    if (modelIndex >= man->modelLen)
    {
        fprintf(stderr, "[disableModel] modelIndex %u out of range (modelLen=%u)\n", modelIndex, man->modelLen);
        return;
    }

    Model3D* model = &man->pModel[modelIndex];

    if (model->alive == 0)
    {
        fprintf(stderr, "[disableModel] model %u is permanently destroyed, cannot disable\n", modelIndex);
        return;
    }

    model->alive = 2;
}

void enableModel(Models3DManager* man, uint32_t modelIndex)
{
    if (modelIndex >= man->modelLen)
    {
        fprintf(stderr, "[enableModel] modelIndex %u out of range (modelLen=%u)\n", modelIndex, man->modelLen);
        return;
    }

    Model3D* model = &man->pModel[modelIndex];

    if (model->alive == 0)
    {
        fprintf(stderr, "[enableModel] model %u is permanently destroyed, cannot re-enable\n", modelIndex);
        return;
    }

    model->alive = 1;
}

// ---------------------------------------------------------------------------
// Runtime model lifecycle — permanent destruction
// ---------------------------------------------------------------------------

void destroyInstance(Models3DManager* man, uint32_t modelIndex, uint32_t instanceIndex)
{
    if (modelIndex >= man->modelLen)
    {
        fprintf(stderr, "[destroyInstance] modelIndex %u out of range (modelLen=%u)\n", modelIndex, man->modelLen);
        return;
    }

    Model3D* model = &man->pModel[modelIndex];

    if (!model->alive)
    {
        fprintf(stderr, "[destroyInstance] model %u is permanently destroyed\n", modelIndex);
        return;
    }

    if (instanceIndex >= model->instanceCount)
    {
        fprintf(stderr, "[destroyInstance] instanceIndex %u out of range (instanceCount=%u)\n", instanceIndex,
                model->instanceCount);
        return;
    }

    Model3DInstance* inst = &man->pDenseModelInstances[model->denseBaseOffset + instanceIndex];

    if (inst->alive == INSTANCE_DESTROYED)
    {
        fprintf(stderr, "[destroyInstance] instance %u of model %u is already destroyed\n", instanceIndex, modelIndex);
        return;
    }

    inst->alive = INSTANCE_DESTROYED;
}

void destroyModel(Models3DManager* man, uint32_t modelIndex)
{
    if (modelIndex >= man->modelLen)
    {
        fprintf(stderr, "[destroyModel] modelIndex %u out of range (modelLen=%u)\n", modelIndex, man->modelLen);
        return;
    }

    Model3D* model = &man->pModel[modelIndex];

    if (!model->alive)
    {
        return;
    }

    // Mark as permanently dead — instances stay in dense array as dead slots
    model->alive = 0;
}

int queryInstanceAlive(const Models3DManager* man, uint32_t modelIndex, uint32_t instanceIndex)
{
    if (modelIndex >= man->modelLen)
        return -1;

    const Model3D* model = &man->pModel[modelIndex];
    if (!model->alive || instanceIndex >= model->instanceCount)
        return -1;

    return man->pDenseModelInstances[model->denseBaseOffset + instanceIndex].alive;
}

int queryModelAlive(const Models3DManager* man, uint32_t modelIndex)
{
    if (modelIndex >= man->modelLen)
        return -1;

    return man->pModel[modelIndex].alive;
}

// ---------------------------------------------------------------------------
// Runtime light lifecycle — instance-level toggle
// ---------------------------------------------------------------------------

void disableLightInstance(LightManager* man, uint32_t lightIndex, uint32_t instanceIndex)
{
    if (lightIndex >= man->lightLen)
    {
        fprintf(stderr, "[disableLightInstance] lightIndex %u out of range (lightLen=%u)\n", lightIndex, man->lightLen);
        return;
    }

    LightVolume* light = &man->pLight[lightIndex];

    if (!light->alive)
    {
        fprintf(stderr, "[disableLightInstance] light %u is permanently destroyed\n", lightIndex);
        return;
    }

    if (instanceIndex >= light->lightVolumeInstanceCount)
    {
        fprintf(stderr, "[disableLightInstance] instanceIndex %u out of range (count=%u)\n", instanceIndex,
                light->lightVolumeInstanceCount);
        return;
    }

    LightVolumeInstance* inst = &light->pLightVolumeInstance[instanceIndex];

    if (inst->alive != LIGHT_INSTANCE_ALIVE)
    {
        return;
    }

    inst->alive = LIGHT_INSTANCE_DISABLED;
}

void enableLightInstance(LightManager* man, uint32_t lightIndex, uint32_t instanceIndex)
{
    if (lightIndex >= man->lightLen)
    {
        fprintf(stderr, "[enableLightInstance] lightIndex %u out of range (lightLen=%u)\n", lightIndex, man->lightLen);
        return;
    }

    LightVolume* light = &man->pLight[lightIndex];

    if (!light->alive)
    {
        fprintf(stderr, "[enableLightInstance] light %u is permanently destroyed, cannot re-enable\n", lightIndex);
        return;
    }

    if (instanceIndex >= light->lightVolumeInstanceCount)
    {
        fprintf(stderr, "[enableLightInstance] instanceIndex %u out of range (count=%u)\n", instanceIndex,
                light->lightVolumeInstanceCount);
        return;
    }

    LightVolumeInstance* inst = &light->pLightVolumeInstance[instanceIndex];

    if (inst->alive == LIGHT_INSTANCE_DESTROYED)
    {
        fprintf(stderr, "[enableLightInstance] instance %u of light %u is permanently destroyed, cannot re-enable\n",
                instanceIndex, lightIndex);
        return;
    }

    inst->alive = LIGHT_INSTANCE_ALIVE;
}

// ---------------------------------------------------------------------------
// Runtime light lifecycle — light-level toggle
// ---------------------------------------------------------------------------

void disableLight(LightManager* man, uint32_t lightIndex)
{
    if (lightIndex >= man->lightLen)
    {
        fprintf(stderr, "[disableLight] lightIndex %u out of range (lightLen=%u)\n", lightIndex, man->lightLen);
        return;
    }

    LightVolume* light = &man->pLight[lightIndex];

    if (light->alive == LIGHT_DESTROYED)
    {
        fprintf(stderr, "[disableLight] light %u is permanently destroyed, cannot disable\n", lightIndex);
        return;
    }

    light->alive = LIGHT_DISABLED;
}

void enableLight(LightManager* man, uint32_t lightIndex)
{
    if (lightIndex >= man->lightLen)
    {
        fprintf(stderr, "[enableLight] lightIndex %u out of range (lightLen=%u)\n", lightIndex, man->lightLen);
        return;
    }

    LightVolume* light = &man->pLight[lightIndex];

    if (light->alive == LIGHT_DESTROYED)
    {
        fprintf(stderr, "[enableLight] light %u is permanently destroyed, cannot re-enable\n", lightIndex);
        return;
    }

    light->alive = LIGHT_ALIVE;
}

// ---------------------------------------------------------------------------
// Runtime light lifecycle — permanent destruction
// ---------------------------------------------------------------------------

void destroyLightInstance(LightManager* man, uint32_t lightIndex, uint32_t instanceIndex)
{
    if (lightIndex >= man->lightLen)
    {
        fprintf(stderr, "[destroyLightInstance] lightIndex %u out of range (lightLen=%u)\n", lightIndex, man->lightLen);
        return;
    }

    LightVolume* light = &man->pLight[lightIndex];

    if (!light->alive)
    {
        fprintf(stderr, "[destroyLightInstance] light %u is permanently destroyed\n", lightIndex);
        return;
    }

    if (instanceIndex >= light->lightVolumeInstanceCount)
    {
        fprintf(stderr, "[destroyLightInstance] instanceIndex %u out of range (count=%u)\n", instanceIndex,
                light->lightVolumeInstanceCount);
        return;
    }

    LightVolumeInstance* inst = &light->pLightVolumeInstance[instanceIndex];

    if (inst->alive == LIGHT_INSTANCE_DESTROYED)
    {
        fprintf(stderr, "[destroyLightInstance] instance %u of light %u is already destroyed\n", instanceIndex,
                lightIndex);
        return;
    }

    inst->alive = LIGHT_INSTANCE_DESTROYED;
}

void destroyLight(LightManager* man, uint32_t lightIndex)
{
    if (lightIndex >= man->lightLen)
    {
        fprintf(stderr, "[destroyLight] lightIndex %u out of range (lightLen=%u)\n", lightIndex, man->lightLen);
        return;
    }

    LightVolume* light = &man->pLight[lightIndex];

    if (!light->alive)
    {
        return;
    }

    light->alive = LIGHT_DESTROYED;

    if (light->pLightVolumeInstance)
    {
        free(light->pLightVolumeInstance);
        light->pLightVolumeInstance     = NULL;
        light->lightVolumeInstanceCount = 0;
    }
}

// ---------------------------------------------------------------------------
// Runtime light lifecycle — queries
// ---------------------------------------------------------------------------

int queryLightInstanceAlive(const LightManager* man, uint32_t lightIndex, uint32_t instanceIndex)
{
    if (lightIndex >= man->lightLen)
        return -1;

    const LightVolume* light = &man->pLight[lightIndex];
    if (!light->pLightVolumeInstance || instanceIndex >= light->lightVolumeInstanceCount)
        return -1;

    return light->pLightVolumeInstance[instanceIndex].alive;
}

int queryLightAlive(const LightManager* man, uint32_t lightIndex)
{
    if (lightIndex >= man->lightLen)
        return -1;

    return man->pLight[lightIndex].alive;
}
