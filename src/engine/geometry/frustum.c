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
// Per-frame frustum cull pass
// ---------------------------------------------------------------------------

void frustumCullScene(SCENE* scene, const FrustumPlanes* frustum)
{
    uint32_t totalTested = 0;
    uint32_t totalCulled = 0;

    // Cull model instances
    for (uint32_t modelIdx = 0; modelIdx < scene->modelsManager.modelLen; modelIdx++)
    {
        Model3D* model = &scene->modelsManager.pModel[modelIdx];

        if (model->alive != MODEL_ALIVE)
            continue;

        for (uint32_t i = 0; i < model->instanceCount; i++)
        {
            Model3DInstance* inst = &scene->modelsManager.pDenseModelInstances[model->denseBaseOffset + i];

            if (inst->lifecycle != INSTANCE_ALIVE)
                continue;

            TransformNode* tnode = &scene->transformManager.pTransforms[inst->transformIdx];

            // Already disabled by hierarchy cascade — don't override
            if (tnode->passFlags == TRANSFORM_PASS_DISABLED)
                continue;

            totalTested++;
            if (!frustumTestAABB(frustum, &model->localAABB, tnode->worldMatrix))
            {
                tnode->passFlags = TRANSFORM_PASS_DISABLED;
                totalCulled++;
            }
        }
    }

#ifdef DEBUG
    static uint32_t frameCounter = 0;
    if (++frameCounter % 3 == 0)
    {
        fprintf(stderr, "  [frustum] tested=%u culled=%u visible=%u\n",
                totalTested, totalCulled, totalTested - totalCulled);
    }
#endif

    // Cull light instances
    for (uint32_t lightIdx = 0; lightIdx < scene->lightManager.lightLen; lightIdx++)
    {
        LightVolume* light = &scene->lightManager.pLight[lightIdx];

        if (light->alive != LIGHT_ALIVE)
            continue;

        for (uint32_t i = 0; i < light->lightVolumeInstanceCount; i++)
        {
            LightVolumeInstance* inst = &scene->lightManager.pDenseLightInstances[light->denseBaseOffset + i];

            if (inst->lifecycle != LIGHT_INSTANCE_ALIVE)
                continue;

            TransformNode* tnode = &scene->transformManager.pTransforms[inst->transformIdx];

            if (tnode->passFlags == TRANSFORM_PASS_DISABLED)
                continue;

            if (!frustumTestAABB(frustum, &light->localAABB, tnode->worldMatrix))
            {
                tnode->passFlags = TRANSFORM_PASS_DISABLED;
            }
        }
    }
}
