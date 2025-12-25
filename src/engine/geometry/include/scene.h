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
#include "gpuDrivenCommandGen.h"

typedef struct SCENE
{

    // Geometry data for render buffers
    LightManager    lightManager;
    Models3DManager modelsManager;
    uint32_t        sceneInstancesLen;

    ParticleGroupManager particleManager;
    uint32_t             particleGroupsLen;
    // RenderableObject*    renderables;
    // uint32_t             packedRenderablesCount;

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
