#include <stdint.h>
#include <stdio.h>
#include "transformNode.h"
#include "cglm.h"

void transformInitCold(TransformCold* cold)
{
    glm_vec3_zero(cold->position);
    glm_quat_identity(cold->rotation);
    glm_vec3_one(cold->scale);
    glm_mat4_identity(cold->localMatrix);
    cold->_pad1 = 0.0f;
    cold->_pad2 = 0.0f;
}

void transformInitHot(TransformHot* hot)
{
    glm_mat4_identity(hot->worldMatrix);
    hot->parentIdx = TRANSFORM_NO_PARENT;
    hot->lifecycle = TRANSFORM_ALIVE;
    hot->passFlags = 0;
}

void transformInit(TransformManager* man, uint32_t idx)
{
    transformInitHot(&man->pTransformsHot[idx]);
    transformInitCold(&man->pTransformsCold[idx]);
}

void transformSetPosition(TransformCold* cold, vec3 pos)
{
    glm_vec3_copy(pos, cold->position);
}

void transformSetRotation(TransformCold* cold, versor q)
{
    glm_quat_copy(q, cold->rotation);
}

void transformSetRotationEuler(TransformCold* cold, float pitchDeg, float yawDeg, float rollDeg)
{
    versor qPitch, qYaw, qRoll, temp;
    glm_quatv(qPitch, glm_rad(pitchDeg), (vec3){1.0f, 0.0f, 0.0f});
    glm_quatv(qYaw,   glm_rad(yawDeg),   (vec3){0.0f, 1.0f, 0.0f});
    glm_quatv(qRoll,  glm_rad(rollDeg),   (vec3){0.0f, 0.0f, 1.0f});

    glm_quat_mul(qYaw, qPitch, temp);
    glm_quat_mul(temp, qRoll, cold->rotation);
}

void transformSetScale(TransformCold* cold, vec3 scale)
{
    glm_vec3_copy(scale, cold->scale);
}

void transformSetScaleUniform(TransformCold* cold, float s)
{
    cold->scale[0] = s;
    cold->scale[1] = s;
    cold->scale[2] = s;
}

void transformTranslate(TransformCold* cold, vec3 delta)
{
    glm_vec3_add(cold->position, delta, cold->position);
}

void transformRotateAxis(TransformCold* cold, float angleDeg, vec3 axis)
{
    versor deltaQ;
    glm_quatv(deltaQ, glm_rad(angleDeg), axis);
    versor result;
    glm_quat_mul(deltaQ, cold->rotation, result);
    glm_quat_normalize_to(result, cold->rotation);
}

void transformBuildLocalMatrix(TransformCold* cold)
{
    // localMatrix = T * R * S
    mat4 T, R, S, temp;

    glm_translate_make(T, cold->position);
    glm_quat_mat4(cold->rotation, R);
    glm_scale_make(S, cold->scale);

    glm_mat4_mul(T, R, temp);
    glm_mat4_mul(temp, S, cold->localMatrix);
}

void transformSetParent(TransformManager* man, uint32_t childIdx, uint32_t parentIdx)
{
    if (childIdx >= man->count)
    {
        fprintf(stderr, "[transformSetParent] childIdx %u out of range (count=%u)\n",
                childIdx, man->count);
        return;
    }
    if (parentIdx >= man->count)
    {
        fprintf(stderr, "[transformSetParent] parentIdx %u out of range (count=%u)\n",
                parentIdx, man->count);
        return;
    }
    if (parentIdx >= childIdx)
    {
        fprintf(stderr,
                "[transformSetParent] rejected: parent %u must be < child %u "
                "(parents must precede children in the dense array)\n",
                parentIdx, childIdx);
        return;
    }

    man->pTransformsHot[childIdx].parentIdx = parentIdx;
}

void transformClearParent(TransformManager* man, uint32_t idx)
{
    if (idx >= man->count)
    {
        fprintf(stderr, "[transformClearParent] idx %u out of range (count=%u)\n",
                idx, man->count);
        return;
    }

    man->pTransformsHot[idx].parentIdx = TRANSFORM_NO_PARENT;
}

void resolveTransforms(TransformManager* man)
{
    uint32_t live = 0;

    for (uint32_t i = 0; i < man->count; i++)
    {
        TransformHot*  hot  = &man->pTransformsHot[i];
        TransformCold* cold = &man->pTransformsCold[i];

        // Clear per-frame pass flags
        hot->passFlags = 0;

        // Skip destroyed / disabled nodes (mirror lifecycle into passFlags)
        if (hot->lifecycle == TRANSFORM_DESTROYED || hot->lifecycle == TRANSFORM_DISABLED)
        {
            hot->passFlags = TRANSFORM_PASS_DISABLED;
            continue;
        }

        // Rebuild local matrix from TRS (cold → cold)
        transformBuildLocalMatrix(cold);

        if (hot->parentIdx != TRANSFORM_NO_PARENT)
        {
            TransformHot* parentHot = &man->pTransformsHot[hot->parentIdx];

            // Cascade-disable: if parent is not renderable this frame, child isn't either
            if (parentHot->passFlags == TRANSFORM_PASS_DISABLED)
            {
                hot->passFlags = TRANSFORM_PASS_DISABLED;
                continue;
            }

            // worldMatrix = parent.worldMatrix * localMatrix
            glm_mat4_mul(parentHot->worldMatrix, cold->localMatrix, hot->worldMatrix);
        }
        else
        {
            // Root node: worldMatrix = localMatrix
            glm_mat4_copy(cold->localMatrix, hot->worldMatrix);
        }

        live++;
    }

    man->liveCount = live;
}
