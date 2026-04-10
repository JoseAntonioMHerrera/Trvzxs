#include <stdint.h>
#include "transform.h"
#include "cglm.h"

void instanceInitTransform(TransformManager* tman, Model3DInstance* inst)
{
    transformInit(&tman->pTransforms[inst->transformIdx]);
}

void instanceSetPosition(TransformManager* tman, Model3DInstance* inst, vec3 pos)
{
    transformSetPosition(&tman->pTransforms[inst->transformIdx], pos);
}

void instanceSetRotation(TransformManager* tman, Model3DInstance* inst, versor q)
{
    transformSetRotation(&tman->pTransforms[inst->transformIdx], q);
}

void instanceSetRotationEuler(TransformManager* tman, Model3DInstance* inst, float pitchDeg, float yawDeg, float rollDeg)
{
    transformSetRotationEuler(&tman->pTransforms[inst->transformIdx], pitchDeg, yawDeg, rollDeg);
}

void instanceSetScale(TransformManager* tman, Model3DInstance* inst, vec3 scale)
{
    transformSetScale(&tman->pTransforms[inst->transformIdx], scale);
}

void instanceSetScaleUniform(TransformManager* tman, Model3DInstance* inst, float scale)
{
    transformSetScaleUniform(&tman->pTransforms[inst->transformIdx], scale);
}

void instanceTranslate(TransformManager* tman, Model3DInstance* inst, vec3 delta)
{
    transformTranslate(&tman->pTransforms[inst->transformIdx], delta);
}

void instanceRotateAxis(TransformManager* tman, Model3DInstance* inst, float angleDeg, vec3 axis)
{
    transformRotateAxis(&tman->pTransforms[inst->transformIdx], angleDeg, axis);
}

uint32_t computeGlobalInstanceIndex(Model3D* model, uint32_t localIdx)
{
    return model->denseBaseOffset + localIdx;
}

Model3DInstance* resolveGlobalInstance(Models3DManager* man, uint32_t globalIdx)
{
    return &man->pDenseModelInstances[globalIdx];
}

