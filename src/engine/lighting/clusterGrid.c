#include "clusterGrid.h"
#include <math.h>
#include <float.h>

// ---------------------------------------------------------------------------
// Clustered forward+ — CPU cluster AABB builder.
//
// Per cluster (x, y, z):
//   - NDC XY bounds: [-1 + 2x/X, -1 + 2(x+1)/X] × [-1 + 2y/Y, -1 + 2(y+1)/Y]
//     (Vulkan clip-space is [-1,1] in XY; Y flip is already baked into proj.)
//   - View-space Z slab: [zN(z), zN(z+1)] with zN(k) = near*(far/near)^(k/Z).
//   - Unproject the 4 NDC XY corners at NDC-z=0 to get view-space rays from
//     the origin. Intersect each ray with view planes Z = zN(z) and Z = zN(z+1)
//     to obtain 8 view-space points. Reduce to min/max → AABB.
// ---------------------------------------------------------------------------

static void unprojectNdc(mat4 invProj, float nx, float ny, float nz, vec3 outView)
{
    vec4 clip  = {nx, ny, nz, 1.0f};
    vec4 viewH = {0};
    glm_mat4_mulv(invProj, clip, viewH);
    float invW = 1.0f / viewH[3];
    outView[0] = viewH[0] * invW;
    outView[1] = viewH[1] * invW;
    outView[2] = viewH[2] * invW;
}

// Intersect ray from origin through dir with view plane z = targetZ.
// proj is left-handed Vulkan [0,1]; forward is +Z, so near<far with both positive.
static void intersectRayWithZPlane(const vec3 dir, float targetZ, vec3 out)
{
    // dir.z is expected non-zero for any NDC point that isn't on the YZ plane.
    float t = targetZ / dir[2];
    out[0]  = dir[0] * t;
    out[1]  = dir[1] * t;
    out[2]  = targetZ;
}

static float zSliceNear(int k, float nearZ, float farZ)
{
    // Logarithmic slice boundary: zN(k) = near * (far/near)^(k/Z).
    return nearZ * powf(farZ / nearZ, (float)k / (float)CLUSTER_Z);
}

int clusterSliceFromViewZ(float viewZ, float nearZ, float farZ)
{
    if (viewZ <= nearZ)
        return 0;
    if (viewZ >= farZ)
        return CLUSTER_Z - 1;
    // Inverse of zN(k): k = log(viewZ/near) * Z / log(far/near).
    float s = logf(viewZ / nearZ) * (float)CLUSTER_Z / logf(farZ / nearZ);
    int   i = (int)floorf(s);
    if (i < 0)
        i = 0;
    if (i >= CLUSTER_Z)
        i = CLUSTER_Z - 1;
    return i;
}

void buildClusterAABBs(mat4 proj, float nearZ, float farZ, ClusterAABB out[CLUSTER_COUNT])
{
    mat4 invProj;
    glm_mat4_inv(proj, invProj);

    // Precompute the 4 view-space rays for each XY tile: one unprojected point
    // per NDC corner at ndc-z=0. Cache per row so we reuse across Z slices.
    // 17 x 10 unique corner points — allocate on stack.
    vec3 cornerRay[CLUSTER_X + 1][CLUSTER_Y + 1];
    for (int y = 0; y <= CLUSTER_Y; ++y)
    {
        float ny = -1.0f + 2.0f * (float)y / (float)CLUSTER_Y;
        for (int x = 0; x <= CLUSTER_X; ++x)
        {
            float nx = -1.0f + 2.0f * (float)x / (float)CLUSTER_X;
            unprojectNdc(invProj, nx, ny, 0.0f, cornerRay[x][y]);
        }
    }

    for (int z = 0; z < CLUSTER_Z; ++z)
    {
        float zN = zSliceNear(z, nearZ, farZ);
        float zF = zSliceNear(z + 1, nearZ, farZ);

        for (int y = 0; y < CLUSTER_Y; ++y)
        {
            for (int x = 0; x < CLUSTER_X; ++x)
            {
                vec3 mn = {FLT_MAX, FLT_MAX, FLT_MAX};
                vec3 mx = {-FLT_MAX, -FLT_MAX, -FLT_MAX};

                for (int cy = 0; cy < 2; ++cy)
                {
                    for (int cx = 0; cx < 2; ++cx)
                    {
                        const float* ray = cornerRay[x + cx][y + cy];
                        vec3         pN, pF;
                        intersectRayWithZPlane(ray, zN, pN);
                        intersectRayWithZPlane(ray, zF, pF);

                        for (int a = 0; a < 3; ++a)
                        {
                            if (pN[a] < mn[a])
                                mn[a] = pN[a];
                            if (pF[a] < mn[a])
                                mn[a] = pF[a];
                            if (pN[a] > mx[a])
                                mx[a] = pN[a];
                            if (pF[a] > mx[a])
                                mx[a] = pF[a];
                        }
                    }
                }

                int          idx = x + y * CLUSTER_X + z * CLUSTER_X * CLUSTER_Y;
                ClusterAABB* c   = &out[idx];
                c->minView[0]    = mn[0];
                c->minView[1]    = mn[1];
                c->minView[2]    = mn[2];
                c->minView[3]    = 0.0f;
                c->maxView[0]    = mx[0];
                c->maxView[1]    = mx[1];
                c->maxView[2]    = mx[2];
                c->maxView[3]    = 0.0f;
            }
        }
    }
}
