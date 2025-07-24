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

typedef struct GpuMappedMemory {
    uint32_t* modelIndices;

} GpuMappedMemory;
typedef struct SCENE {

    // Geometry data for render buffers
    LightManager lightManager;
    Models3DManager modelsManager;
    uint32_t sceneInstancesLen;

    ParticleGroupManager particleManager;
    uint32_t particleGroupsLen;

    GpuMappedMemory gpuMem;

} SCENE;

uint32_t getIndicesLenFromScene(const SCENE * const scene);
uint32_t* getIndicesFromScene(const SCENE * const scene);
uint32_t getVertexLenFromScene(const SCENE * const scene);

GEOM_VERTEX* getVertexFromScene(const SCENE * const scene);

void initScene(SCENE** scene, uint32_t modelsLen, uint32_t volLightsLen);
void fillSceneWithModelData(SCENE* scene, Model3D* pModel, uint32_t modelLen);
void fillSceneWithLightData(SCENE* scene, LightVolume* pLight, uint32_t volumetricLightsLen);
void printSceneModels(const Models3DManager* const modelManager);
void printSceneLights(const LightManager* const volManager);
void printLightVolume(LightVolume* const light);
void fillSceneWithInstancedModelData(Models3DManager* man, Model3DInstance** instanceData, uint32_t totalInstanceDataLen);
void fillSceneWithInstancedLightVolumeData(LightManager* lan, LightVolumeInstance** instanceData, uint32_t totalInstanceDataLen);
