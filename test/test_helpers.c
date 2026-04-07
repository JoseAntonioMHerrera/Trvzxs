#include "test_helpers.h"
#include "model.h"
#include "light.h"
#include "transform.h"
#include "transformNode.h"
#include <stdlib.h>
#include <string.h>

TransformManager make_test_transform_manager(uint32_t count)
{
    TransformManager tman;
    memset(&tman, 0, sizeof(tman));
    tman.count      = count;
    tman.liveCount  = count;
    tman.pTransforms = (TransformNode*) calloc(count, sizeof(TransformNode));
    for (uint32_t i = 0; i < count; i++)
        transformInit(&tman.pTransforms[i]);
    return tman;
}

void free_test_transform_manager(TransformManager* tman)
{
    if (!tman)
        return;
    free(tman->pTransforms);
    tman->pTransforms = NULL;
    tman->count       = 0;
    tman->liveCount   = 0;
}

// Internal: sets up model manager with transforms starting at transformBase
static Models3DManager make_test_manager_internal(uint32_t modelCount, const uint32_t* instanceCounts,
                                                   TransformManager* tman, uint32_t transformBase)
{
    Models3DManager man;
    memset(&man, 0, sizeof(man));

    man.pModel   = (Model3D*) calloc(modelCount, sizeof(Model3D));
    man.modelLen = modelCount;

    uint32_t totalInstances = 0;
    for (uint32_t m = 0; m < modelCount; m++)
        totalInstances += instanceCounts[m];

    man.modelInstanceCapacity = totalInstances;
    man.pDenseModelInstances  = (Model3DInstance*) calloc(totalInstances, sizeof(Model3DInstance));

    uint32_t denseOffset = 0;
    for (uint32_t m = 0; m < modelCount; m++)
    {
        Model3D* model = &man.pModel[m];

        model->instanceCount      = instanceCounts[m];
        model->liveInstanceCount  = instanceCounts[m];
        model->modelInstanceCount = instanceCounts[m];
        model->alive              = MODEL_ALIVE;
        model->denseBaseOffset    = denseOffset;
        model->pStagingInstances  = NULL;
        model->pStagingTransforms = NULL;

        for (uint32_t i = 0; i < instanceCounts[m]; i++)
        {
            Model3DInstance* inst = &man.pDenseModelInstances[denseOffset + i];
            inst->transformIdx = transformBase + denseOffset + i;
            inst->lifecycle    = INSTANCE_ALIVE;
            inst->color.r = 1.0f;
            inst->color.g = 1.0f;
            inst->color.b = 1.0f;
            inst->color.a = 1.0f;
        }

        denseOffset += instanceCounts[m];
    }
    man.modelInstanceLen         = totalInstances;
    man.instanceOffset.items     = (uint32_t*) calloc(modelCount, sizeof(uint32_t));
    man.instanceOffset.itemsLen  = modelCount;

    uint32_t offset = 0;
    for (uint32_t m = 0; m < modelCount; m++)
    {
        man.instanceOffset.items[m] = offset;
        offset += instanceCounts[m];
    }

    return man;
}

Models3DManager make_test_manager(uint32_t modelCount, uint32_t instancesPerModel)
{
    // Allocate a temporary array of counts
    uint32_t* counts = (uint32_t*) malloc(modelCount * sizeof(uint32_t));
    for (uint32_t m = 0; m < modelCount; m++)
        counts[m] = instancesPerModel;

    // Note: this version doesn't allocate transforms — callers must use make_test_scene
    // or set up their own TransformManager for UBO-fill tests.
    // For lifecycle-only tests that don't call UBO fill, transforms aren't needed.
    Models3DManager man = make_test_manager_internal(modelCount, counts, NULL, 0);

    free(counts);
    return man;
}

Models3DManager make_test_manager_varied(uint32_t modelCount, const uint32_t* instanceCounts)
{
    return make_test_manager_internal(modelCount, instanceCounts, NULL, 0);
}

void free_test_manager(Models3DManager* man)
{
    if (!man)
        return;

    free(man->pDenseModelInstances);
    free(man->pModel);
    free(man->instanceOffset.items);

    man->pDenseModelInstances = NULL;
    man->pModel               = NULL;
    man->instanceOffset.items = NULL;
    man->modelLen             = 0;
}

LightManager make_test_light_manager(uint32_t lightCount, uint32_t instancesPerLight)
{
    LightManager man;
    memset(&man, 0, sizeof(man));

    man.pLight   = (LightVolume*) calloc(lightCount, sizeof(LightVolume));
    man.lightLen = lightCount;

    uint32_t totalInstances = lightCount * instancesPerLight;

    man.lightInstanceCapacity  = totalInstances;
    man.lightInstanceCount     = totalInstances;
    man.pDenseLightInstances   = (LightVolumeInstance*) calloc(totalInstances, sizeof(LightVolumeInstance));
    man.instanceOffset.items   = (uint32_t*) calloc(lightCount, sizeof(uint32_t));
    man.instanceOffset.itemsLen = lightCount;

    uint32_t offset = 0;
    for (uint32_t l = 0; l < lightCount; l++)
    {
        LightVolume* light = &man.pLight[l];

        light->denseBaseOffset          = offset;
        light->lightVolumeInstanceCount = instancesPerLight;
        light->liveInstanceCount        = instancesPerLight;
        light->alive                    = LIGHT_ALIVE;
        light->pStagingInstances        = NULL;
        light->pStagingTransforms       = NULL;

        for (uint32_t i = 0; i < instancesPerLight; i++)
        {
            man.pDenseLightInstances[offset + i].lifecycle    = LIGHT_INSTANCE_ALIVE;
            man.pDenseLightInstances[offset + i].transformIdx = 0; // set by scene helpers
            man.pDenseLightInstances[offset + i].color.r  = 1.0f;
            man.pDenseLightInstances[offset + i].color.g  = 1.0f;
            man.pDenseLightInstances[offset + i].color.b  = 1.0f;
            man.pDenseLightInstances[offset + i].color.a  = 1.0f;
        }

        man.instanceOffset.items[l] = offset;
        offset += instancesPerLight;
    }

    return man;
}

void free_test_light_manager(LightManager* man)
{
    if (!man)
        return;

    free(man->pDenseLightInstances);
    free(man->pLight);
    free(man->instanceOffset.items);

    man->pLight               = NULL;
    man->instanceOffset.items = NULL;
    man->lightLen             = 0;
}

// Internal: sets up light manager with transforms starting at transformBase
static LightManager make_test_light_manager_internal(uint32_t lightCount, uint32_t instancesPerLight,
                                                      TransformManager* tman, uint32_t transformBase)
{
    LightManager man;
    memset(&man, 0, sizeof(man));

    man.pLight   = (LightVolume*) calloc(lightCount, sizeof(LightVolume));
    man.lightLen = lightCount;

    uint32_t totalInstances = lightCount * instancesPerLight;

    man.lightInstanceCapacity  = totalInstances;
    man.lightInstanceCount     = totalInstances;
    man.pDenseLightInstances   = (LightVolumeInstance*) calloc(totalInstances, sizeof(LightVolumeInstance));
    man.instanceOffset.items   = (uint32_t*) calloc(lightCount, sizeof(uint32_t));
    man.instanceOffset.itemsLen = lightCount;

    uint32_t offset = 0;
    for (uint32_t l = 0; l < lightCount; l++)
    {
        LightVolume* light = &man.pLight[l];

        light->denseBaseOffset          = offset;
        light->lightVolumeInstanceCount = instancesPerLight;
        light->liveInstanceCount        = instancesPerLight;
        light->alive                    = LIGHT_ALIVE;
        light->pStagingInstances        = NULL;
        light->pStagingTransforms       = NULL;

        for (uint32_t i = 0; i < instancesPerLight; i++)
        {
            LightVolumeInstance* inst = &man.pDenseLightInstances[offset + i];
            inst->lifecycle    = LIGHT_INSTANCE_ALIVE;
            inst->transformIdx = transformBase + offset + i;
            inst->color.r      = 1.0f;
            inst->color.g      = 1.0f;
            inst->color.b      = 1.0f;
            inst->color.a      = 1.0f;
        }

        man.instanceOffset.items[l] = offset;
        offset += instancesPerLight;
    }

    return man;
}

SCENE* make_test_scene_mixed(uint32_t modelCount, uint32_t instancesPerModel,
                             uint32_t lightCount, uint32_t instancesPerLight)
{
    SCENE* scene = (SCENE*) calloc(1, sizeof(SCENE));

    uint32_t totalModelInstances = modelCount * instancesPerModel;
    uint32_t totalLightInstances = lightCount * instancesPerLight;
    uint32_t totalTransforms     = totalModelInstances + totalLightInstances;

    // Single shared TransformManager
    scene->transformManager = make_test_transform_manager(totalTransforms);

    // Models occupy transform indices [0, totalModelInstances)
    uint32_t* modelCounts = (uint32_t*) malloc(modelCount * sizeof(uint32_t));
    for (uint32_t m = 0; m < modelCount; m++)
        modelCounts[m] = instancesPerModel;

    scene->modelsManager = make_test_manager_internal(modelCount, modelCounts, &scene->transformManager, 0);
    free(modelCounts);

    // Lights occupy transform indices [totalModelInstances, totalTransforms)
    scene->lightManager = make_test_light_manager_internal(lightCount, instancesPerLight,
                                                           &scene->transformManager, totalModelInstances);

    scene->camera.eye[0]    = 0.0f;
    scene->camera.eye[1]    = 0.0f;
    scene->camera.eye[2]    = 5.0f;
    scene->camera.center[0] = 0.0f;
    scene->camera.center[1] = 0.0f;
    scene->camera.center[2] = 0.0f;
    scene->camera.up[0]     = 0.0f;
    scene->camera.up[1]     = 1.0f;
    scene->camera.up[2]     = 0.0f;

    return scene;
}

void free_test_scene_mixed(SCENE* scene)
{
    if (!scene)
        return;

    free_test_manager(&scene->modelsManager);
    free_test_light_manager(&scene->lightManager);
    free_test_transform_manager(&scene->transformManager);
    free(scene);
}

SCENE* make_test_scene(uint32_t modelCount, uint32_t instancesPerModel)
{
    SCENE* scene = (SCENE*) calloc(1, sizeof(SCENE));

    uint32_t totalInstances = modelCount * instancesPerModel;

    // Allocate transforms for model instances
    scene->transformManager = make_test_transform_manager(totalInstances);

    // Build model manager with transforms
    uint32_t* counts = (uint32_t*) malloc(modelCount * sizeof(uint32_t));
    for (uint32_t m = 0; m < modelCount; m++)
        counts[m] = instancesPerModel;

    scene->modelsManager = make_test_manager_internal(modelCount, counts, &scene->transformManager, 0);

    free(counts);

    scene->camera.eye[0]    = 0.0f;
    scene->camera.eye[1]    = 0.0f;
    scene->camera.eye[2]    = 5.0f;
    scene->camera.center[0] = 0.0f;
    scene->camera.center[1] = 0.0f;
    scene->camera.center[2] = 0.0f;
    scene->camera.up[0]     = 0.0f;
    scene->camera.up[1]     = 1.0f;
    scene->camera.up[2]     = 0.0f;

    return scene;
}

void free_test_scene(SCENE* scene)
{
    if (!scene)
        return;

    free_test_manager(&scene->modelsManager);
    free_test_transform_manager(&scene->transformManager);
    free(scene);
}
