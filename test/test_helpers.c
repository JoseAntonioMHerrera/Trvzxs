#include "test_helpers.h"
#include "model.h"
#include <stdlib.h>
#include <string.h>

Models3DManager make_test_manager(uint32_t modelCount, uint32_t instancesPerModel)
{
    Models3DManager man;
    memset(&man, 0, sizeof(man));

    man.pModel   = (Model3D*) calloc(modelCount, sizeof(Model3D));
    man.modelLen = modelCount;

    uint32_t totalInstances = 0;

    for (uint32_t m = 0; m < modelCount; m++)
    {
        Model3D* model = &man.pModel[m];

        model->pModelInstance     = (Model3DInstance*) calloc(instancesPerModel, sizeof(Model3DInstance));
        model->instanceCount      = instancesPerModel;
        model->liveInstanceCount  = instancesPerModel;
        model->modelInstanceCount = instancesPerModel;
        model->alive              = MODEL_ALIVE;

        for (uint32_t i = 0; i < instancesPerModel; i++)
        {
            model->pModelInstance[i].alive = INSTANCE_ALIVE;
            glm_mat4_identity(model->pModelInstance[i].m);
            model->pModelInstance[i].color.r = 1.0f;
            model->pModelInstance[i].color.g = 1.0f;
            model->pModelInstance[i].color.b = 1.0f;
            model->pModelInstance[i].color.a = 1.0f;
        }

        totalInstances += instancesPerModel;
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

void free_test_manager(Models3DManager* man)
{
    if (!man)
        return;

    for (uint32_t m = 0; m < man->modelLen; m++)
    {
        free(man->pModel[m].pModelInstance);
    }
    free(man->pModel);
    free(man->instanceOffset.items);

    man->pModel              = NULL;
    man->instanceOffset.items = NULL;
    man->modelLen             = 0;
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
