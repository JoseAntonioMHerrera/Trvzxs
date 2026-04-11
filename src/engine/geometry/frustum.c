#include "frustum.h"
#include "scene.h"
#include "transformNode.h"
#include "vertex.h"
#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdio.h>

// ---------------------------------------------------------------------------
// Gribb/Hartmann plane extraction from VP matrix (Vulkan [0,1] depth range)
// ---------------------------------------------------------------------------
// cglm is column-major: m[col][row].
// Row i of the matrix = (m[0][i], m[1][i], m[2][i], m[3][i]).
//
// Plane equations (normals point inward):
//   Left   = row3 + row0     (-w' <= x')
//   Right  = row3 - row0     (x' <= w')
//   Bottom = row3 + row1     (-w' <= y')
//   Top    = row3 - row1     (y' <= w')
//   Near   = row2            (0  <= z')    <-- Vulkan, not row3+row2
//   Far    = row3 - row2     (z' <= w')

static void extractRow(mat4 m, int row, vec4 out)
{
    out[0] = m[0][row];
    out[1] = m[1][row];
    out[2] = m[2][row];
    out[3] = m[3][row];
}

static void normalizePlane(vec4 p)
{
    float len = sqrtf(p[0] * p[0] + p[1] * p[1] + p[2] * p[2]);
    if (len > 1e-8f)
    {
        float inv = 1.0f / len;
        p[0] *= inv;
        p[1] *= inv;
        p[2] *= inv;
        p[3] *= inv;
    }
}

void frustumExtractPlanes(mat4 viewProj, FrustumPlanes* out)
{
    vec4 r0, r1, r2, r3;
    extractRow(viewProj, 0, r0);
    extractRow(viewProj, 1, r1);
    extractRow(viewProj, 2, r2);
    extractRow(viewProj, 3, r3);

    // Left: row3 + row0
    for (int i = 0; i < 4; i++)
        out->planes[0][i] = r3[i] + r0[i];

    // Right: row3 - row0
    for (int i = 0; i < 4; i++)
        out->planes[1][i] = r3[i] - r0[i];

    // Bottom: row3 + row1
    for (int i = 0; i < 4; i++)
        out->planes[2][i] = r3[i] + r1[i];

    // Top: row3 - row1
    for (int i = 0; i < 4; i++)
        out->planes[3][i] = r3[i] - r1[i];

    // Near: row2 (Vulkan [0,1] depth)
    for (int i = 0; i < 4; i++)
        out->planes[4][i] = r2[i];

    // Far: row3 - row2
    for (int i = 0; i < 4; i++)
        out->planes[5][i] = r3[i] - r2[i];

    for (int i = 0; i < 6; i++)
        normalizePlane(out->planes[i]);
}

// ---------------------------------------------------------------------------
// Arvo AABB transform + p-vertex frustum test
// ---------------------------------------------------------------------------

bool frustumTestAABB(const FrustumPlanes* frustum, const AABB* localAABB, mat4 worldMatrix)
{
    // Arvo's method: transform AABB to world space without computing 8 corners.
    // worldMatrix is column-major m[col][row].
    vec3 worldMin, worldMax;

    for (int i = 0; i < 3; i++)
    {
        // Start with translation component
        worldMin[i] = worldMatrix[3][i];
        worldMax[i] = worldMatrix[3][i];

        for (int j = 0; j < 3; j++)
        {
            if (worldMatrix[j][i] >= 0.0f)
            {
                worldMin[i] += worldMatrix[j][i] * localAABB->min[j];
                worldMax[i] += worldMatrix[j][i] * localAABB->max[j];
            }
            else
            {
                worldMin[i] += worldMatrix[j][i] * localAABB->max[j];
                worldMax[i] += worldMatrix[j][i] * localAABB->min[j];
            }
        }
    }

    // P-vertex test: for each plane, pick the AABB corner most aligned with the normal.
    // If that corner is outside, the entire box is outside.
    for (int i = 0; i < 6; i++)
    {
        float a = frustum->planes[i][0];
        float b = frustum->planes[i][1];
        float c = frustum->planes[i][2];
        float d = frustum->planes[i][3];

        float px = (a >= 0.0f) ? worldMax[0] : worldMin[0];
        float py = (b >= 0.0f) ? worldMax[1] : worldMin[1];
        float pz = (c >= 0.0f) ? worldMax[2] : worldMin[2];

        if (a * px + b * py + c * pz + d < 0.0f)
            return false; // Entire AABB is outside this plane
    }

    return true; // AABB is inside (or intersects) all 6 planes
}

// ---------------------------------------------------------------------------
// AABB computation from vertex array
// ---------------------------------------------------------------------------

void computeAABBFromVertices(const GEOM_VERTEX* vertices, uint32_t vertexCount, AABB* out)
{
    assert(vertices != NULL);
    assert(out != NULL);
    assert(vertexCount > 0);

    out->min[0] = FLT_MAX;
    out->min[1] = FLT_MAX;
    out->min[2] = FLT_MAX;
    out->max[0] = -FLT_MAX;
    out->max[1] = -FLT_MAX;
    out->max[2] = -FLT_MAX;

    for (uint32_t i = 0; i < vertexCount; i++)
    {
        float x = vertices[i].point.x;
        float y = vertices[i].point.y;
        float z = vertices[i].point.z;

        if (x < out->min[0])
            out->min[0] = x;
        if (y < out->min[1])
            out->min[1] = y;
        if (z < out->min[2])
            out->min[2] = z;

        if (x > out->max[0])
            out->max[0] = x;
        if (y > out->max[1])
            out->max[1] = y;
        if (z > out->max[2])
            out->max[2] = z;
    }
}

// ---------------------------------------------------------------------------
// Per-frame frustum cull pass (preorder walk + subtree bulk-skip)
// ---------------------------------------------------------------------------
//
// Iterates pTransformsHot[] in preorder order rather than per-type. Uses:
//   - pTransformsHot[i].passFlags as the authoritative "visible this frame"
//     signal (set by resolveTransforms for lifecycle/cascade disables, and
//     by this function for frustum misses).
//   - pPreorderEntities[i] to find the backing per-type instance (needed to
//     check per-instance lifecycle — the instance's DESTROYED/DISABLED state
//     is not mirrored onto the transform).
//   - pLocalAABBs[i] as the per-entity local-space AABB for the frustum test
//     (copied from the type's Model3D.localAABB / LightVolume.localAABB at
//     load time).
//
// Subtree bulk-skip semantics:
//   - If a node is already passFlags==DISABLED (set by resolve for lifecycle
//     or cascade), we can skip its entire subtree in O(1): resolve has
//     already propagated the flag to every descendant.
//   - If a node fails the frustum test, we must propagate passFlags down its
//     subtree via an O(skipCount) loop before advancing, because resolve
//     never saw this decision. This is still a huge win: we pay skipCount
//     writes instead of skipCount frustum tests.

static inline void markSubtreeDisabled(TransformHot* hot, uint32_t rootIdx)
{
    uint32_t skip = hot[rootIdx].skipCount;
    for (uint32_t j = rootIdx + 1; j < rootIdx + 1 + skip; j++)
    {
        hot[j].passFlags |= TRANSFORM_PASS_DISABLED;
    }
}

void frustumCullScene(SCENE* scene, const FrustumPlanes* frustum)
{
    TransformManager* tman = &scene->transformManager;
    uint32_t          totalTested = 0;
    uint32_t          totalCulled = 0;

    uint32_t i = 0;
    while (i < tman->count)
    {
        TransformHot* hot = &tman->pTransformsHot[i];

        // Fast path: already disabled by resolve (lifecycle or cascade).
        // Descendants are already correctly flagged — bulk-skip in O(1).
        if (hot->passFlags & TRANSFORM_PASS_DISABLED)
        {
            i += 1 + hot->skipCount;
            continue;
        }

        // Check backing instance state via the preorder reverse index.
        // Per-instance lifecycle lives on the dense instance, not the
        // transform, so we must cross the indirection here.
        PreorderEntity* entity = &tman->pPreorderEntities[i];
        int             alive  = 1;
        if ((SceneEntityKind) entity->kind == SCENE_ENTITY_MODEL)
        {
            Model3D* type = &scene->modelsManager.pModel[entity->typeIdx];
            if (type->alive != MODEL_ALIVE)
            {
                alive = 0;
            }
            else
            {
                Model3DInstance* inst = &scene->modelsManager.pDenseModelInstances[entity->denseIdx];
                if (inst->lifecycle != INSTANCE_ALIVE)
                    alive = 0;
            }
        }
        else // SCENE_ENTITY_LIGHT
        {
            LightVolume* type = &scene->lightManager.pLight[entity->typeIdx];
            if (type->alive != LIGHT_ALIVE)
            {
                alive = 0;
            }
            else
            {
                LightVolumeInstance* inst = &scene->lightManager.pDenseLightInstances[entity->denseIdx];
                if (inst->lifecycle != LIGHT_INSTANCE_ALIVE)
                    alive = 0;
            }
        }

        if (!alive)
        {
            // Instance-level disable: mark this node + propagate to subtree.
            hot->passFlags |= TRANSFORM_PASS_DISABLED;
            markSubtreeDisabled(tman->pTransformsHot, i);
            i += 1 + hot->skipCount;
            continue;
        }

        // Actual frustum test against the per-entity local AABB.
        totalTested++;
        if (!frustumTestAABB(frustum, &tman->pLocalAABBs[i], tman->pTransformsWorld[i].worldMatrix))
        {
            hot->passFlags |= TRANSFORM_PASS_DISABLED;
            totalCulled++;
            markSubtreeDisabled(tman->pTransformsHot, i);
            i += 1 + hot->skipCount;
            continue;
        }

        i++;
    }

#ifdef DEBUG
    static uint32_t frameCounter = 0;
    if (++frameCounter % 3 == 0)
    {
        fprintf(stderr, "  [frustum] tested=%u culled=%u visible=%u\n",
                totalTested, totalCulled, totalTested - totalCulled);
    }
#endif
}
