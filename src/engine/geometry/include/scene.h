#pragma once

#include <stdint.h>
#include "cglm.h"
#include "numberTypes.h"
#include "vertex.h"
#include "assert.h"
#include "array.h"
#include "light.h"
#include "model.h"
#include "particle.h"

typedef struct SCENE
{

    // Geometry data for render buffers
    LightManager    lightManager;
    Models3DManager modelsManager;
    uint32_t        sceneInstancesLen;

    ParticleGroupManager particleManager;
    uint32_t             particleGroupsLen;

    Camera3D camera;

} SCENE;

uint32_t  getIndicesLenFromScene(const SCENE* const scene);
uint32_t* getIndicesFromScene(const SCENE* const scene);
uint32_t  getVertexLenFromScene(const SCENE* const scene);

GEOM_VERTEX* getVertexFromScene(const SCENE* const scene);

void initScene(SCENE** scene, uint32_t modelsLen, uint32_t lightsLen, uint32_t particleVerticesLen,
               uint32_t particleGroupsLen, uint32_t particleInstancesLen);
void fillSceneWithModelData(SCENE* scene, Model3D* pModel, uint32_t modelLen);
void fillSceneWithLightData(SCENE* scene, LightVolume* pLight, uint32_t volumetricLightsLen);
void fillSceneWithParticleData(SCENE* scene, ParticleGroup* particleGroups, uint32_t particleGroupsLen,
                               TrzVertex* particleVertices, uint32_t particleVerticesCount,
                               uint32_t* particleVertexIndices, uint32_t particleVertexIndicesLen);
void printSceneModels(const Models3DManager* const modelManager);
void printSceneLights(const LightManager* const volManager);
void printLightVolume(LightVolume* const light);
void fillSceneWithInstancedModelData(Models3DManager* man, Model3DInstance** instanceData,
                                     uint32_t totalInstanceDataLen);
void fillSceneWithInstancedLightVolumeData(LightManager* lan, LightVolumeInstance** instanceData,
                                           uint32_t totalInstanceDataLen);

// --- Runtime model lifecycle: instance-level toggle ---

// Disable a single instance: sets alive = 0.
void disableInstance(Models3DManager* man, uint32_t modelIndex, uint32_t instanceIndex);

// Re-enable a disabled instance: sets alive = 1.
// Refuses if the instance is permanently destroyed (alive == 2).
void enableInstance(Models3DManager* man, uint32_t modelIndex, uint32_t instanceIndex);

// --- Runtime model lifecycle: model-level toggle ---

// Disable all instances of a model: sets all instance alive flags to 0.
void disableModel(Models3DManager* man, uint32_t modelIndex);

// Re-enable all instances of a model: sets all instance alive flags to 1.
// No-op if the model was permanently destroyed (alive == 0).
void enableModel(Models3DManager* man, uint32_t modelIndex);

// --- Runtime model lifecycle: permanent destruction ---

// Permanently destroy a single instance: sets alive = INSTANCE_DESTROYED.
// The slot remains as wasted space. Cannot be re-enabled.
void destroyInstance(Models3DManager* man, uint32_t modelIndex, uint32_t instanceIndex);

// Permanently destroy a model: marks model alive = MODEL_DESTROYED,
// frees CPU-side instance data. Cannot be re-enabled.
void destroyModel(Models3DManager* man, uint32_t modelIndex);

// --- Runtime model lifecycle: queries ---

// Returns the alive state of an instance, or -1 if the model/instance index
// is out of range or the model is destroyed (pModelInstance freed).
int queryInstanceAlive(const Models3DManager* man, uint32_t modelIndex, uint32_t instanceIndex);

// Returns the alive state of a model, or -1 if modelIndex is out of range.
int queryModelAlive(const Models3DManager* man, uint32_t modelIndex);
