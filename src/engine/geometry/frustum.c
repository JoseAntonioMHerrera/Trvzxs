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
// Sphere vs frustum test (cheaper, conservative)
// ---------------------------------------------------------------------------

bool frustumTestSphere(const FrustumPlanes* frustum, const vec3 worldCenter, float worldRadius)
{
    for (int i = 0; i < 6; i++)
    {
        float d = frustum->planes[i][0] * worldCenter[0] +
                  frustum->planes[i][1] * worldCenter[1] +
                  frustum->planes[i][2] * worldCenter[2] +
                  frustum->planes[i][3];
        if (d < -worldRadius)
            return false;
    }
    return true;
}

// Derive a world-space bounding sphere from a local AABB + worldMatrix.
// Precondition: AABB is centered at local origin (recenterVerticesAroundOrigin
// runs at load). This makes worldMatrix[3] == world-space AABB center exactly,
// so no vec3 transform is needed and the radius is the branchless half-diagonal.
//   center = worldMatrix[3].xyz
//   radius = ||halfExtent|| × max(||col0||, ||col1||, ||col2||)
// Uniform scale → exact; non-uniform scale → conservative.
static inline void sphereFromAABB(const AABB* localAABB, mat4 worldMatrix, vec3 outCenter, float* outRadius)
{
    outCenter[0] = worldMatrix[3][0];
    outCenter[1] = worldMatrix[3][1];
    outCenter[2] = worldMatrix[3][2];

    float hx = 0.5f * (localAABB->max[0] - localAABB->min[0]);
    float hy = 0.5f * (localAABB->max[1] - localAABB->min[1]);
    float hz = 0.5f * (localAABB->max[2] - localAABB->min[2]);
    float localRadiusSq = hx * hx + hy * hy + hz * hz;

    float s0Sq = worldMatrix[0][0] * worldMatrix[0][0] + worldMatrix[0][1] * worldMatrix[0][1] +
                 worldMatrix[0][2] * worldMatrix[0][2];
    float s1Sq = worldMatrix[1][0] * worldMatrix[1][0] + worldMatrix[1][1] * worldMatrix[1][1] +
                 worldMatrix[1][2] * worldMatrix[1][2];
    float s2Sq = worldMatrix[2][0] * worldMatrix[2][0] + worldMatrix[2][1] * worldMatrix[2][1] +
                 worldMatrix[2][2] * worldMatrix[2][2];
    float maxScaleSq = s0Sq;
    if (s1Sq > maxScaleSq) maxScaleSq = s1Sq;
    if (s2Sq > maxScaleSq) maxScaleSq = s2Sq;

    *outRadius = sqrtf(localRadiusSq * maxScaleSq);
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

void recenterVerticesAroundOrigin(GEOM_VERTEX* vertices, uint32_t vertexCount, AABB* aabb)
{
    assert(vertices != NULL);
    assert(aabb != NULL);
    assert(vertexCount > 0);

    float cx = 0.5f * (aabb->min[0] + aabb->max[0]);
    float cy = 0.5f * (aabb->min[1] + aabb->max[1]);
    float cz = 0.5f * (aabb->min[2] + aabb->max[2]);

    for (uint32_t i = 0; i < vertexCount; i++)
    {
        vertices[i].point.x -= cx;
        vertices[i].point.y -= cy;
        vertices[i].point.z -= cz;
    }

    aabb->min[0] -= cx;
    aabb->min[1] -= cy;
    aabb->min[2] -= cz;
    aabb->max[0] -= cx;
    aabb->max[1] -= cy;
    aabb->max[2] -= cz;
}

// ---------------------------------------------------------------------------
// Per-frame frustum cull pass (preorder walk + subtree bulk-skip)
// ---------------------------------------------------------------------------
//
// Runs BEFORE resolveTransforms. Consumes last-frame's worldMatrix against
// this-frame's frustum and writes TRANSFORM_PASS_FRUSTUM_CULLED. Resolve then
// bulk-skips any node carrying FRUSTUM_CULLED (or DISABLED from last frame),
// saving the dirty-branch matrix rebuild for offscreen nodes. One-frame
// staleness is bounded and explicit.
//
// Uses:
//   - pTransformsHot[i].passFlags — DISABLED from last-frame resolve serves
//     as the lifecycle/cascade fast-path skip; FRUSTUM_CULLED is this pass's
//     output.
//   - pPreorderEntities[i] to reach the backing per-type instance for the
//     per-instance lifecycle check.
//   - pLocalAABBs[i] as the local-space AABB.
//
// Dirty-override:
//   A DIRTY node's worldMatrix is stale by definition, so the frustum test
//   would lie. We clear FRUSTUM_CULLED on dirty nodes unconditionally — they
//   fall through to resolve, which refreshes their matrix. The one-frame
//   staleness window only applies to clean nodes, whose matrices are still
//   accurate.
//
// Subtree bulk-skip semantics:
//   - DISABLED fast path (lifecycle/cascade from last frame): O(1) skip, no
//     writes — resolve already propagated.
//   - FRUSTUM_CULLED miss: propagate FRUSTUM_CULLED through the subtree
//     via O(skipCount), skipping DIRTY descendants so they still resolve.

static inline void markSubtreeCulled(TransformHot* hot, uint32_t rootIdx)
{
    uint32_t skip = hot[rootIdx].skipCount;
    for (uint32_t j = rootIdx + 1; j < rootIdx + 1 + skip; j++)
    {
        // Don't mask DIRTY descendants — their matrices are stale and must
        // resolve. Resolve will decide visibility next frame.
        if (!(hot[j].passFlags & TRANSFORM_PASS_DIRTY))
            hot[j].passFlags |= TRANSFORM_PASS_FRUSTUM_CULLED;
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

        // Fast path: lifecycle/cascade disabled from last-frame resolve.
        // Descendants already carry DISABLED — bulk-skip in O(1). Leave
        // FRUSTUM_CULLED bit alone (resolve won't look at it when DISABLED
        // is set, and we don't have a valid frustum answer for these).
        if (hot->passFlags & TRANSFORM_PASS_DISABLED)
        {
            i += 1 + hot->skipCount;
            continue;
        }

        // Dirty-override: stale worldMatrix, can't trust the frustum test.
        // Force-clear FRUSTUM_CULLED so resolve processes this node.
        if (hot->passFlags & TRANSFORM_PASS_DIRTY)
        {
            hot->passFlags &= ~TRANSFORM_PASS_FRUSTUM_CULLED;
            i++;
            continue;
        }

        // Per-instance lifecycle (via preorder reverse index) — catches
        // instances that died since last frame's resolve ran.
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
            hot->passFlags |= TRANSFORM_PASS_FRUSTUM_CULLED;
            markSubtreeCulled(tman->pTransformsHot, i);
            i += 1 + hot->skipCount;
            continue;
        }

        // Frustum test against last-frame worldMatrix.
        totalTested++;
        if (!frustumTestAABB(frustum, &tman->pLocalAABBs[i], tman->pTransformsWorld[i].worldMatrix))
        {
            hot->passFlags |= TRANSFORM_PASS_FRUSTUM_CULLED;
            totalCulled++;
            markSubtreeCulled(tman->pTransformsHot, i);
            i += 1 + hot->skipCount;
            continue;
        }

        // Visible: clear any stale FRUSTUM_CULLED from a previous frame.
        hot->passFlags &= ~TRANSFORM_PASS_FRUSTUM_CULLED;
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

// ---------------------------------------------------------------------------
// Sphere-variant cull pass — same control flow as frustumCullScene, only the
// geometric test differs. Kept as a parallel function for clean A/B swapping.
// ---------------------------------------------------------------------------

void frustumCullSceneSphere(SCENE* scene, const FrustumPlanes* frustum)
{
    TransformManager* tman = &scene->transformManager;
    uint32_t          totalTested = 0;
    uint32_t          totalCulled = 0;

    uint32_t i = 0;
    while (i < tman->count)
    {
        TransformHot* hot = &tman->pTransformsHot[i];

        if (hot->passFlags & TRANSFORM_PASS_DISABLED)
        {
            i += 1 + hot->skipCount;
            continue;
        }

        if (hot->passFlags & TRANSFORM_PASS_DIRTY)
        {
            hot->passFlags &= ~TRANSFORM_PASS_FRUSTUM_CULLED;
            i++;
            continue;
        }

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
        else
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
            hot->passFlags |= TRANSFORM_PASS_FRUSTUM_CULLED;
            markSubtreeCulled(tman->pTransformsHot, i);
            i += 1 + hot->skipCount;
            continue;
        }

        vec3  worldCenter;
        float worldRadius;
        sphereFromAABB(&tman->pLocalAABBs[i], tman->pTransformsWorld[i].worldMatrix, worldCenter, &worldRadius);

        totalTested++;
        if (!frustumTestSphere(frustum, worldCenter, worldRadius))
        {
            hot->passFlags |= TRANSFORM_PASS_FRUSTUM_CULLED;
            totalCulled++;
            markSubtreeCulled(tman->pTransformsHot, i);
            i += 1 + hot->skipCount;
            continue;
        }

        hot->passFlags &= ~TRANSFORM_PASS_FRUSTUM_CULLED;
        i++;
    }

#ifdef DEBUG
    static uint32_t frameCounter = 0;
    if (++frameCounter % 3 == 0)
    {
        fprintf(stderr, "  [frustum-sphere] tested=%u culled=%u visible=%u\n",
                totalTested, totalCulled, totalTested - totalCulled);
    }
#endif
}
