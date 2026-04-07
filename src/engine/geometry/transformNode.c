#include <stdint.h>
#include <stdio.h>
#include "transformNode.h"
#include "cglm.h"

void transformInit(TransformNode* node)
{
    glm_vec3_zero(node->position);
    glm_quat_identity(node->rotation);
    glm_vec3_one(node->scale);
    glm_mat4_identity(node->localMatrix);
    glm_mat4_identity(node->worldMatrix);
    node->parentIdx = TRANSFORM_NO_PARENT;
    node->lifecycle  = TRANSFORM_ALIVE;
    node->passFlags  = 0;
}

void transformSetPosition(TransformNode* node, vec3 pos)
{
    glm_vec3_copy(pos, node->position);
}

void transformSetRotation(TransformNode* node, versor q)
{
    glm_quat_copy(q, node->rotation);
}

void transformSetRotationEuler(TransformNode* node, float pitchDeg, float yawDeg, float rollDeg)
{
    versor qPitch, qYaw, qRoll, temp;
    glm_quatv(qPitch, glm_rad(pitchDeg), (vec3){1.0f, 0.0f, 0.0f});
    glm_quatv(qYaw,   glm_rad(yawDeg),   (vec3){0.0f, 1.0f, 0.0f});
    glm_quatv(qRoll,  glm_rad(rollDeg),   (vec3){0.0f, 0.0f, 1.0f});

    glm_quat_mul(qYaw, qPitch, temp);
    glm_quat_mul(temp, qRoll, node->rotation);
}

void transformSetScale(TransformNode* node, vec3 scale)
{
    glm_vec3_copy(scale, node->scale);
}

void transformSetScaleUniform(TransformNode* node, float s)
{
    node->scale[0] = s;
    node->scale[1] = s;
    node->scale[2] = s;
}

void transformTranslate(TransformNode* node, vec3 delta)
{
    glm_vec3_add(node->position, delta, node->position);
}

void transformRotateAxis(TransformNode* node, float angleDeg, vec3 axis)
{
    versor deltaQ;
    glm_quatv(deltaQ, glm_rad(angleDeg), axis);
    versor result;
    glm_quat_mul(deltaQ, node->rotation, result);
    glm_quat_normalize_to(result, node->rotation);
}

void transformBuildLocalMatrix(TransformNode* node)
{
    // localMatrix = T * R * S
    mat4 T, R, S, temp;

    glm_translate_make(T, node->position);
    glm_quat_mat4(node->rotation, R);
    glm_scale_make(S, node->scale);

    glm_mat4_mul(T, R, temp);
    glm_mat4_mul(temp, S, node->localMatrix);
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

    man->pTransforms[childIdx].parentIdx = parentIdx;
}

void transformClearParent(TransformManager* man, uint32_t idx)
{
    if (idx >= man->count)
    {
        fprintf(stderr, "[transformClearParent] idx %u out of range (count=%u)\n",
                idx, man->count);
        return;
    }

    man->pTransforms[idx].parentIdx = TRANSFORM_NO_PARENT;
}

void resolveTransforms(TransformManager* man)
{
    uint32_t live = 0;

    for (uint32_t i = 0; i < man->count; i++)
    {
        TransformNode* node = &man->pTransforms[i];

        // Clear per-frame pass flags
        node->passFlags = 0;

        // Skip destroyed nodes
        if (node->lifecycle == TRANSFORM_DESTROYED)
        {
            node->passFlags = TRANSFORM_PASS_DISABLED;
            continue;
        }

        // Skip disabled nodes
        if (node->lifecycle == TRANSFORM_DISABLED)
        {
            node->passFlags = TRANSFORM_PASS_DISABLED;
            continue;
        }

        // Build local matrix from TRS
        transformBuildLocalMatrix(node);

        if (node->parentIdx != TRANSFORM_NO_PARENT)
        {
            TransformNode* parent = &man->pTransforms[node->parentIdx];

            // Cascade-disable: if parent is not renderable this frame, child isn't either
            if (parent->passFlags == TRANSFORM_PASS_DISABLED)
            {
                node->passFlags = TRANSFORM_PASS_DISABLED;
                continue;
            }

            // worldMatrix = parent.worldMatrix * localMatrix
            glm_mat4_mul(parent->worldMatrix, node->localMatrix, node->worldMatrix);
        }
        else
        {
            // Root node: worldMatrix = localMatrix
            glm_mat4_copy(node->localMatrix, node->worldMatrix);
        }

        live++;
    }

    man->liveCount = live;
}
