#include <stdint.h>
#include <stdio.h>
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

void instanceSetParent(TransformManager* tman, Models3DManager* man,
                       uint32_t childModelIdx, uint32_t childLocalIdx,
                       uint32_t parentModelIdx, uint32_t parentLocalIdx)
{
    if (childModelIdx >= man->modelLen)
    {
        fprintf(stderr, "[instanceSetParent] childModelIdx %u out of range (modelLen=%u)\n",
                childModelIdx, man->modelLen);
        return;
    }
    if (parentModelIdx >= man->modelLen)
    {
        fprintf(stderr, "[instanceSetParent] parentModelIdx %u out of range (modelLen=%u)\n",
                parentModelIdx, man->modelLen);
        return;
    }
    if (childLocalIdx >= man->pModel[childModelIdx].instanceCount)
    {
        fprintf(stderr, "[instanceSetParent] childLocalIdx %u out of range (instanceCount=%u)\n",
                childLocalIdx, man->pModel[childModelIdx].instanceCount);
        return;
    }
    if (parentLocalIdx >= man->pModel[parentModelIdx].instanceCount)
    {
        fprintf(stderr, "[instanceSetParent] parentLocalIdx %u out of range (instanceCount=%u)\n",
                parentLocalIdx, man->pModel[parentModelIdx].instanceCount);
        return;
    }

    Model3DInstance* child  = &man->pDenseModelInstances[man->pModel[childModelIdx].denseBaseOffset + childLocalIdx];
    Model3DInstance* parent = &man->pDenseModelInstances[man->pModel[parentModelIdx].denseBaseOffset + parentLocalIdx];

    transformSetParent(tman, child->transformIdx, parent->transformIdx);
}

void instanceClearParent(TransformManager* tman, Models3DManager* man, uint32_t modelIdx, uint32_t localIdx)
{
    if (modelIdx >= man->modelLen)
    {
        fprintf(stderr, "[instanceClearParent] modelIdx %u out of range (modelLen=%u)\n",
                modelIdx, man->modelLen);
        return;
    }
    if (localIdx >= man->pModel[modelIdx].instanceCount)
    {
        fprintf(stderr, "[instanceClearParent] localIdx %u out of range (instanceCount=%u)\n",
                localIdx, man->pModel[modelIdx].instanceCount);
        return;
    }

    Model3DInstance* inst = &man->pDenseModelInstances[man->pModel[modelIdx].denseBaseOffset + localIdx];
    transformClearParent(tman, inst->transformIdx);
}
