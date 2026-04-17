#include <stdint.h>
#include <stdio.h>
#include "transformNode.h"
#include "cglm.h"

// Set by camera cuts / scene reloads to force one full resolve pass that
// ignores FRUSTUM_CULLED. Consumed + cleared by resolveTransforms.
int g_visibilityCacheInvalidated = 1;

void transformInitCold(TransformCold* cold)
{
    glm_vec3_zero(cold->position);
    glm_quat_identity(cold->rotation);
    glm_vec3_one(cold->scale);
    cold->_pad1 = 0.0f;
    cold->_pad2 = 0.0f;
}

void transformInitHot(TransformHot* hot)
{
    hot->parentIdx = TRANSFORM_NO_PARENT;
    hot->lifecycle = TRANSFORM_ALIVE;
    // Every newly-initialized slot starts dirty so the first resolve pass
    // computes its worldMatrix from the (just-assigned) TRS.
    hot->passFlags = TRANSFORM_PASS_DIRTY;
    hot->skipCount = 0;
}

void transformInitWorld(TransformWorld* world)
{
    glm_mat4_identity(world->worldMatrix);
}

void transformInit(TransformManager* man, uint32_t idx)
{
    transformInitHot(&man->pTransformsHot[idx]);
    transformInitWorld(&man->pTransformsWorld[idx]);
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

void transformBuildLocalMatrix(const TransformCold* cold, mat4 outLocal)
{
    mat4 T, R, S, temp;

    glm_translate_make(T, (float*)cold->position);
    glm_quat_mat4((float*)cold->rotation, R);
    glm_scale_make(S, (float*)cold->scale);

    glm_mat4_mul(T, R, temp);
    glm_mat4_mul(temp, S, outLocal);
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
    TransformHot*   pHot   = man->pTransformsHot;
    TransformWorld* pWorld = man->pTransformsWorld;
    TransformCold*  pCold  = man->pTransformsCold;
    uint32_t        count  = man->count;
    uint32_t        live   = 0;

    // Camera cut / scene reload: ignore FRUSTUM_CULLED for exactly one frame,
    // so stale cached matrices can't cause one-frame ghost renders. Clear the
    // flag after consuming it.
    const int invalidate = g_visibilityCacheInvalidated;
    const uint32_t skipMask = invalidate ? TRANSFORM_PASS_DISABLED
                                         : TRANSFORM_RESOLVE_SKIP_MASK;

    // Pass 1: SoA preorder walk. Touches only the 12-byte pHot entries for
    // clean nodes (241 KB at 20K entries — L2-resident). pWorld is loaded
    // only when a node is dirty, i.e. when we actually recompute.
    for (uint32_t i = 0; i < count; i++)
    {
        TransformHot* hot = &pHot[i];

        // Clear DISABLED but keep DIRTY + FRUSTUM_CULLED — DIRTY is sticky
        // until pass 2; FRUSTUM_CULLED was written by this-frame's cull pass
        // which ran immediately before resolve.
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
        uint32_t      parentIdx   = hot->parentIdx;
        if (parentIdx != TRANSFORM_NO_PARENT)
        {
            parentHot = &pHot[parentIdx];
            if (parentHot->passFlags & TRANSFORM_PASS_DISABLED)
            {
                hot->passFlags |= TRANSFORM_PASS_DISABLED;
                continue;
            }
            parentDirty = (parentHot->passFlags & TRANSFORM_PASS_DIRTY) != 0;
        }

        // Frustum-culled (decided by cull before us): skip the TRS rebuild.
        // Keep DIRTY sticky so the node resolves when it comes back in view.
        // Cull already guaranteed this isn't set on a dirty node, except in
        // the cache-invalidated bootstrap case where we ignore the bit.
        if (hot->passFlags & skipMask & TRANSFORM_PASS_FRUSTUM_CULLED)
        {
            live++;
            continue;
        }

        int selfDirty = (hot->passFlags & TRANSFORM_PASS_DIRTY) != 0;
        if (!selfDirty && !parentDirty)
        {
            // Clean: cached worldMatrix from previous frame is still valid.
            // pWorld is NOT loaded for this node — that's the point of SoA.
            // Clear FRUSTUM_CULLED so next-frame cull starts from a clean
            // slate (cull re-decides from scratch).
            hot->passFlags &= ~TRANSFORM_PASS_FRUSTUM_CULLED;
            live++;
            continue;
        }

        // Dirty branch: compose local matrix into a stack temp (no writeback
        // to cold — it's scratch), then compose worldMatrix. Mark this node
        // DIRTY so its children (later in preorder) also recompute. Clear
        // FRUSTUM_CULLED — matrix is fresh, next-frame cull decides anew.
        TransformCold* cold = &pCold[i];
        mat4 localMatrix;
        transformBuildLocalMatrix(cold, localMatrix);

        if (parentHot != NULL)
        {
            glm_mat4_mul(pWorld[parentIdx].worldMatrix, localMatrix, pWorld[i].worldMatrix);
        }
        else
        {
            glm_mat4_copy(localMatrix, pWorld[i].worldMatrix);
        }

        hot->passFlags |= TRANSFORM_PASS_DIRTY;
        hot->passFlags &= ~TRANSFORM_PASS_FRUSTUM_CULLED;
        live++;
    }

    // Pass 2: clear DIRTY only on nodes whose worldMatrix was actually
    // recomputed this frame. Disabled nodes (lifecycle/cascade) and
    // frustum-culled nodes keep DIRTY sticky so the first resolve after they
    // come back alive / into view recomputes from fresh TRS.
    for (uint32_t i = 0; i < count; i++)
    {
        if (!(pHot[i].passFlags & TRANSFORM_RESOLVE_SKIP_MASK))
            pHot[i].passFlags &= ~TRANSFORM_PASS_DIRTY;
    }

    if (invalidate)
        g_visibilityCacheInvalidated = 0;

    man->liveCount = live;
}
