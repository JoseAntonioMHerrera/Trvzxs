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
    // Every newly-initialized slot starts dirty so the first resolve pass
    // computes its worldMatrix from the (just-assigned) TRS.
    hot->passFlags = TRANSFORM_PASS_DIRTY;
    hot->skipCount = 0;
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

    man->pTransformsHot[childIdx].parentIdx  = parentIdx;
    // Reparenting invalidates the child's cached worldMatrix.
    man->pTransformsHot[childIdx].passFlags |= TRANSFORM_PASS_DIRTY;
}

void transformClearParent(TransformManager* man, uint32_t idx)
{
    if (idx >= man->count)
    {
        fprintf(stderr, "[transformClearParent] idx %u out of range (count=%u)\n",
                idx, man->count);
        return;
    }

    man->pTransformsHot[idx].parentIdx  = TRANSFORM_NO_PARENT;
    // Becoming a root invalidates the child's cached worldMatrix.
    man->pTransformsHot[idx].passFlags |= TRANSFORM_PASS_DIRTY;
}

void transformMarkDirty(TransformManager* man, uint32_t idx)
{
    if (idx >= man->count)
    {
        fprintf(stderr, "[transformMarkDirty] idx %u out of range (count=%u)\n",
                idx, man->count);
        return;
    }
    man->pTransformsHot[idx].passFlags |= TRANSFORM_PASS_DIRTY;
}

void resolveTransforms(TransformManager* man)
{
    uint32_t live = 0;

    // Pass 1: preorder walk. Propagate DIRTY through the parent chain, recompute
    // only dirty nodes, leave clean nodes untouched (their worldMatrix from the
    // previous frame is still valid).
    for (uint32_t i = 0; i < man->count; i++)
    {
        TransformHot* hot = &man->pTransformsHot[i];

        // Clear DISABLED but keep DIRTY — DIRTY is sticky until pass 2.
        hot->passFlags &= ~TRANSFORM_PASS_DISABLED;

        // Lifecycle disable (node is DESTROYED or DISABLED)
        if (hot->lifecycle == TRANSFORM_DESTROYED || hot->lifecycle == TRANSFORM_DISABLED)
        {
            hot->passFlags |= TRANSFORM_PASS_DISABLED;
            continue;
        }

        // Cascade disable + parent-dirty lookup
        int           parentDirty = 0;
        TransformHot* parentHot   = NULL;
        if (hot->parentIdx != TRANSFORM_NO_PARENT)
        {
            parentHot = &man->pTransformsHot[hot->parentIdx];
            if (parentHot->passFlags & TRANSFORM_PASS_DISABLED)
            {
                hot->passFlags |= TRANSFORM_PASS_DISABLED;
                continue;
            }
            parentDirty = (parentHot->passFlags & TRANSFORM_PASS_DIRTY) != 0;
        }

        int selfDirty = (hot->passFlags & TRANSFORM_PASS_DIRTY) != 0;
        if (!selfDirty && !parentDirty)
        {
            // Clean: cached worldMatrix from previous frame is still valid.
            live++;
            continue;
        }

        // Dirty: rebuild localMatrix from TRS, compose worldMatrix, and mark
        // this node DIRTY so its children (later in preorder) also recompute.
        TransformCold* cold = &man->pTransformsCold[i];
        transformBuildLocalMatrix(cold);

        if (parentHot != NULL)
        {
            glm_mat4_mul(parentHot->worldMatrix, cold->localMatrix, hot->worldMatrix);
        }
        else
        {
            glm_mat4_copy(cold->localMatrix, hot->worldMatrix);
        }

        hot->passFlags |= TRANSFORM_PASS_DIRTY;
        live++;
    }

    // Pass 2: clear DIRTY on nodes whose worldMatrix was actually recomputed
    // this frame. Disabled nodes (lifecycle or cascade) keep their DIRTY bit
    // sticky so the first resolve after they come back alive recomputes them
    // from fresh TRS instead of skipping them with a stale cached matrix.
    // Streaming write of 2 B per node; the array is already hot in cache
    // from pass 1.
    for (uint32_t i = 0; i < man->count; i++)
    {
        if (!(man->pTransformsHot[i].passFlags & TRANSFORM_PASS_DISABLED))
            man->pTransformsHot[i].passFlags &= ~TRANSFORM_PASS_DIRTY;
    }

    man->liveCount = live;
}
