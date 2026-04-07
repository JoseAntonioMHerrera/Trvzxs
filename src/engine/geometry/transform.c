#include <stdint.h>
#include <stdio.h>
#include "transform.h"
#include "cglm.h"

void instanceInitTransform(Model3DInstance* inst)
{
    glm_vec3_zero(inst->position);
    glm_quat_identity(inst->rotation);
    glm_vec3_one(inst->scale);
    glm_mat4_identity(inst->m);
    inst->parentGlobalIdx = INSTANCE_NO_PARENT;
}

void instanceSetPosition(Model3DInstance* inst, vec3 pos)
{
    glm_vec3_copy(pos, inst->position);
}

void instanceSetRotation(Model3DInstance* inst, versor q)
{
    glm_quat_copy(q, inst->rotation);
}

void instanceSetRotationEuler(Model3DInstance* inst, float pitchDeg, float yawDeg, float rollDeg)
{
    // Build quaternion from euler angles (in radians internally)
    // Rotation order: Y (yaw) * X (pitch) * Z (roll)
    versor qPitch, qYaw, qRoll, temp;
    glm_quatv(qPitch, glm_rad(pitchDeg), (vec3){1.0f, 0.0f, 0.0f});
    glm_quatv(qYaw,   glm_rad(yawDeg),   (vec3){0.0f, 1.0f, 0.0f});
    glm_quatv(qRoll,  glm_rad(rollDeg),   (vec3){0.0f, 0.0f, 1.0f});

    glm_quat_mul(qYaw, qPitch, temp);
    glm_quat_mul(temp, qRoll, inst->rotation);
}

void instanceSetScale(Model3DInstance* inst, vec3 scale)
{
    glm_vec3_copy(scale, inst->scale);
}

void instanceSetScaleUniform(Model3DInstance* inst, float scale)
{
    inst->scale[0] = scale;
    inst->scale[1] = scale;
    inst->scale[2] = scale;
}

void instanceTranslate(Model3DInstance* inst, vec3 delta)
{
    glm_vec3_add(inst->position, delta, inst->position);
}

void instanceRotateAxis(Model3DInstance* inst, float angleDeg, vec3 axis)
{
    versor deltaQ;
    glm_quatv(deltaQ, glm_rad(angleDeg), axis);
    // Compose: new rotation = deltaQ * current rotation
    versor result;
    glm_quat_mul(deltaQ, inst->rotation, result);
    glm_quat_normalize_to(result, inst->rotation);
}

uint32_t computeGlobalInstanceIndex(Model3D* model, uint32_t localIdx)
{
    return model->denseBaseOffset + localIdx;
}

Model3DInstance* resolveGlobalInstance(Models3DManager* man, uint32_t globalIdx)
{
    return &man->pDenseModelInstances[globalIdx];
}

void instanceBuildMatrix(Model3DInstance* inst)
{
    // M = T * R * S
    mat4 T, R, S, temp;

    glm_translate_make(T, inst->position);
    glm_quat_mat4(inst->rotation, R);
    glm_scale_make(S, inst->scale);

    glm_mat4_mul(T, R, temp);
    glm_mat4_mul(temp, S, inst->m);
}

void instanceSetParent(Models3DManager* man,
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

    uint32_t childGlobal  = man->pModel[childModelIdx].denseBaseOffset + childLocalIdx;
    uint32_t parentGlobal = man->pModel[parentModelIdx].denseBaseOffset + parentLocalIdx;

    if (parentGlobal >= childGlobal)
    {
        fprintf(stderr,
                "[instanceSetParent] rejected: parent global %u must be < child global %u "
                "(parents must precede children in the dense array)\n",
                parentGlobal, childGlobal);
        return;
    }

    man->pDenseModelInstances[childGlobal].parentGlobalIdx = parentGlobal;
}

void instanceClearParent(Models3DManager* man, uint32_t modelIdx, uint32_t localIdx)
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

    uint32_t globalIdx = man->pModel[modelIdx].denseBaseOffset + localIdx;
    man->pDenseModelInstances[globalIdx].parentGlobalIdx = INSTANCE_NO_PARENT;
}
