#include "test_helpers.h"
#include "model.h"
#include "light.h"
#include "transform.h"
#include <stdlib.h>
#include <string.h>

Models3DManager make_test_manager(uint32_t modelCount, uint32_t instancesPerModel)
{
    Models3DManager man;
    memset(&man, 0, sizeof(man));

    man.pModel   = (Model3D*) calloc(modelCount, sizeof(Model3D));
    man.modelLen = modelCount;

    uint32_t totalInstances = modelCount * instancesPerModel;

    man.modelInstanceCapacity    = totalInstances;
    man.pDenseModelInstances     = (Model3DInstance*) calloc(totalInstances, sizeof(Model3DInstance));

    uint32_t denseOffset = 0;
    for (uint32_t m = 0; m < modelCount; m++)
    {
        Model3D* model = &man.pModel[m];

        model->instanceCount      = instancesPerModel;
        model->liveInstanceCount  = instancesPerModel;
        model->modelInstanceCount = instancesPerModel;
        model->alive              = MODEL_ALIVE;
        model->denseBaseOffset    = denseOffset;

        for (uint32_t i = 0; i < instancesPerModel; i++)
        {
            Model3DInstance* inst = &man.pDenseModelInstances[denseOffset + i];
            instanceInitTransform(inst);
            inst->alive   = INSTANCE_ALIVE;
            inst->color.r = 1.0f;
            inst->color.g = 1.0f;
            inst->color.b = 1.0f;
            inst->color.a = 1.0f;
        }

        denseOffset += instancesPerModel;
    }
    man.modelInstanceLen         = totalInstances;
    man.instanceOffset.items     = (uint32_t*) calloc(modelCount, sizeof(uint32_t));
    man.instanceOffset.itemsLen  = modelCount;

    uint32_t offset = 0;
    for (uint32_t m = 0; m < modelCount; m++)
    {
        man.instanceOffset.items[m] = offset;
        offset += instancesPerModel;
    }

    return man;
}

Models3DManager make_test_manager_varied(uint32_t modelCount, const uint32_t* instanceCounts)
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

        for (uint32_t i = 0; i < instanceCounts[m]; i++)
        {
            Model3DInstance* inst = &man.pDenseModelInstances[denseOffset + i];
            instanceInitTransform(inst);
            inst->alive   = INSTANCE_ALIVE;
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

    uint32_t totalInstances = 0;

    for (uint32_t l = 0; l < lightCount; l++)
    {
        LightVolume* light = &man.pLight[l];

        light->pLightVolumeInstance =
            (LightVolumeInstance*) calloc(instancesPerLight, sizeof(LightVolumeInstance));
        light->lightVolumeInstanceCount = instancesPerLight;
        light->liveInstanceCount        = instancesPerLight;
        light->alive                    = LIGHT_ALIVE;

        for (uint32_t i = 0; i < instancesPerLight; i++)
        {
            light->pLightVolumeInstance[i].alive   = LIGHT_INSTANCE_ALIVE;
            light->pLightVolumeInstance[i].scale    = 1.0f;
            light->pLightVolumeInstance[i].color.r  = 1.0f;
            light->pLightVolumeInstance[i].color.g  = 1.0f;
            light->pLightVolumeInstance[i].color.b  = 1.0f;
            light->pLightVolumeInstance[i].color.a  = 1.0f;
        }

        totalInstances += instancesPerLight;
    }

    man.lightInstanceCapacity       = totalInstances;
    man.lightInstanceCount          = totalInstances;
    man.instanceOffset.items        = (uint32_t*) calloc(lightCount, sizeof(uint32_t));
    man.instanceOffset.itemsLen     = lightCount;

    uint32_t offset = 0;
    for (uint32_t l = 0; l < lightCount; l++)
    {
        man.instanceOffset.items[l] = offset;
        offset += instancesPerLight;
    }

    return man;
}

void free_test_light_manager(LightManager* man)
{
    if (!man)
        return;

    for (uint32_t l = 0; l < man->lightLen; l++)
    {
        free(man->pLight[l].pLightVolumeInstance);
    }
    free(man->pLight);
    free(man->instanceOffset.items);

    man->pLight               = NULL;
    man->instanceOffset.items = NULL;
    man->lightLen             = 0;
}

SCENE* make_test_scene(uint32_t modelCount, uint32_t instancesPerModel)
{
    SCENE* scene = (SCENE*) calloc(1, sizeof(SCENE));

    scene->modelsManager = make_test_manager(modelCount, instancesPerModel);

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
    free(scene);
}
