#include "test_harness.h"
#include "test_helpers.h"
#include "frustum.h"
#include "scene.h"
#include "transform.h"
#include "transformNode.h"
#include "vertex.h"
#include "cglm.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

// Build a Vulkan-style viewProj matching the engine's convention in
// uniformBuffer.c: lookAt + glm_perspective + Y-flip. Camera at (0,0,5)
// looking at the origin, 45deg fov, square aspect, near 0.1, far 100.
static void make_test_view_proj(mat4 out)
{
    mat4 view, proj;
    glm_lookat((vec3){0.0f, 0.0f, 5.0f}, (vec3){0.0f, 0.0f, 0.0f}, (vec3){0.0f, 1.0f, 0.0f}, view);
    glm_perspective(glm_rad(45.0f), 1.0f, 0.1f, 100.0f, proj);
    proj[1][1] *= -1.0f;
    glm_mat4_mul(proj, view, out);
}

static void translate_world(mat4 out, float x, float y, float z)
{
    glm_mat4_identity(out);
    out[3][0] = x;
    out[3][1] = y;
    out[3][2] = z;
}

static void set_unit_cube_aabb_on_models(SCENE* scene)
{
    for (uint32_t m = 0; m < scene->modelsManager.modelLen; m++)
    {
        Model3D* model = &scene->modelsManager.pModel[m];
        model->localAABB.min[0] = -1.0f;
        model->localAABB.min[1] = -1.0f;
        model->localAABB.min[2] = -1.0f;
        model->localAABB.max[0] =  1.0f;
        model->localAABB.max[1] =  1.0f;
        model->localAABB.max[2] =  1.0f;
    }
}

static void set_unit_cube_aabb_on_lights(SCENE* scene)
{
    for (uint32_t l = 0; l < scene->lightManager.lightLen; l++)
    {
        LightVolume* light = &scene->lightManager.pLight[l];
        light->localAABB.min[0] = -1.0f;
        light->localAABB.min[1] = -1.0f;
        light->localAABB.min[2] = -1.0f;
        light->localAABB.max[0] =  1.0f;
        light->localAABB.max[1] =  1.0f;
        light->localAABB.max[2] =  1.0f;
    }
}

// ---------------------------------------------------------------------------
// 1. computeAABBFromVertices: single vertex collapses to a point
// ---------------------------------------------------------------------------

void test_aabb_single_vertex(void)
{
    GEOM_VERTEX v = {0};
    v.point.x =  3.0f;
    v.point.y = -2.0f;
    v.point.z =  1.5f;

    AABB aabb;
    computeAABBFromVertices(&v, 1, &aabb);

    ASSERT_FLOAT_EQ(aabb.min[0],  3.0f);
    ASSERT_FLOAT_EQ(aabb.min[1], -2.0f);
    ASSERT_FLOAT_EQ(aabb.min[2],  1.5f);
    ASSERT_FLOAT_EQ(aabb.max[0],  3.0f);
    ASSERT_FLOAT_EQ(aabb.max[1], -2.0f);
    ASSERT_FLOAT_EQ(aabb.max[2],  1.5f);
}

// ---------------------------------------------------------------------------
// 2. computeAABBFromVertices: 8 corners of a unit cube
// ---------------------------------------------------------------------------

void test_aabb_unit_cube(void)
{
    GEOM_VERTEX cube[8] = {0};
    int idx = 0;
    for (int x = 0; x < 2; x++)
        for (int y = 0; y < 2; y++)
            for (int z = 0; z < 2; z++)
            {
                cube[idx].point.x = x ? 1.0f : -1.0f;
                cube[idx].point.y = y ? 1.0f : -1.0f;
                cube[idx].point.z = z ? 1.0f : -1.0f;
                idx++;
            }

    AABB aabb;
    computeAABBFromVertices(cube, 8, &aabb);

    ASSERT_FLOAT_EQ(aabb.min[0], -1.0f);
    ASSERT_FLOAT_EQ(aabb.min[1], -1.0f);
    ASSERT_FLOAT_EQ(aabb.min[2], -1.0f);
    ASSERT_FLOAT_EQ(aabb.max[0],  1.0f);
    ASSERT_FLOAT_EQ(aabb.max[1],  1.0f);
    ASSERT_FLOAT_EQ(aabb.max[2],  1.0f);
}

// ---------------------------------------------------------------------------
// 3a. computeAABBFromVertices: postcondition (finite components + min <= max)
//     This is the invariant assertAABBValid() checks at scene load time.
//     If computeAABBFromVertices ever produces non-finite output or an
//     inverted box, the load-time assert will fire and the engine refuses
//     to start — this test is the regression guard on that invariant.
// ---------------------------------------------------------------------------

#define ASSERT_AABB_VALID(aabb)                                                                                        \
    do                                                                                                                 \
    {                                                                                                                  \
        for (int _ax = 0; _ax < 3; _ax++)                                                                              \
        {                                                                                                              \
            if (!isfinite((aabb).min[_ax]) || !isfinite((aabb).max[_ax]))                                              \
            {                                                                                                          \
                printf("FAIL\n    %s:%d: non-finite component on axis %d\n", __FILE__, __LINE__, _ax);                 \
                tests_failed++;                                                                                        \
                return;                                                                                                \
            }                                                                                                          \
            if ((aabb).min[_ax] > (aabb).max[_ax])                                                                     \
            {                                                                                                          \
                printf("FAIL\n    %s:%d: min[%d]=%f > max[%d]=%f\n", __FILE__, __LINE__, _ax, (aabb).min[_ax], _ax,    \
                       (aabb).max[_ax]);                                                                               \
                tests_failed++;                                                                                        \
                return;                                                                                                \
            }                                                                                                          \
        }                                                                                                              \
    } while (0)

void test_aabb_postcondition_finite_and_ordered(void)
{
    AABB aabb;

    // Single vertex (zero extent — valid, min == max on every axis)
    GEOM_VERTEX single = {0};
    single.point.x = 2.0f; single.point.y = 2.0f; single.point.z = 2.0f;
    computeAABBFromVertices(&single, 1, &aabb);
    ASSERT_AABB_VALID(aabb);

    // Unit cube corners
    GEOM_VERTEX cube[8] = {0};
    int idx = 0;
    for (int x = 0; x < 2; x++)
        for (int y = 0; y < 2; y++)
            for (int z = 0; z < 2; z++)
            {
                cube[idx].point.x = x ? 1.0f : -1.0f;
                cube[idx].point.y = y ? 1.0f : -1.0f;
                cube[idx].point.z = z ? 1.0f : -1.0f;
                idx++;
            }
    computeAABBFromVertices(cube, 8, &aabb);
    ASSERT_AABB_VALID(aabb);

    // Skewed extents
    GEOM_VERTEX skew[3] = {0};
    skew[0].point.x = -5.0f; skew[0].point.y = -10.0f; skew[0].point.z = -3.0f;
    skew[1].point.x =  4.0f; skew[1].point.y =   2.0f; skew[1].point.z =  7.0f;
    skew[2].point.x = -2.0f; skew[2].point.y =   8.0f; skew[2].point.z = -1.0f;
    computeAABBFromVertices(skew, 3, &aabb);
    ASSERT_AABB_VALID(aabb);

    // Planar mesh (flat quad on XZ plane, y == 0 → zero extent on Y axis is
    // still a valid postcondition: min[1] == max[1] == 0).
    GEOM_VERTEX quad[4] = {0};
    quad[0].point.x = -1.0f; quad[0].point.y = 0.0f; quad[0].point.z = -1.0f;
    quad[1].point.x =  1.0f; quad[1].point.y = 0.0f; quad[1].point.z = -1.0f;
    quad[2].point.x = -1.0f; quad[2].point.y = 0.0f; quad[2].point.z =  1.0f;
    quad[3].point.x =  1.0f; quad[3].point.y = 0.0f; quad[3].point.z =  1.0f;
    computeAABBFromVertices(quad, 4, &aabb);
    ASSERT_AABB_VALID(aabb);
    ASSERT_FLOAT_EQ(aabb.min[1], 0.0f);
    ASSERT_FLOAT_EQ(aabb.max[1], 0.0f);
}

// ---------------------------------------------------------------------------
// 3. computeAABBFromVertices: asymmetric extents on each axis
// ---------------------------------------------------------------------------

void test_aabb_skewed_extents(void)
{
    GEOM_VERTEX verts[3] = {0};
    verts[0].point.x = -5.0f; verts[0].point.y = -10.0f; verts[0].point.z = -3.0f;
    verts[1].point.x =  4.0f; verts[1].point.y =   2.0f; verts[1].point.z =  7.0f;
    verts[2].point.x = -2.0f; verts[2].point.y =   8.0f; verts[2].point.z = -1.0f;

    AABB aabb;
    computeAABBFromVertices(verts, 3, &aabb);

    ASSERT_FLOAT_EQ(aabb.min[0],  -5.0f);
    ASSERT_FLOAT_EQ(aabb.min[1], -10.0f);
    ASSERT_FLOAT_EQ(aabb.min[2],  -3.0f);
    ASSERT_FLOAT_EQ(aabb.max[0],   4.0f);
    ASSERT_FLOAT_EQ(aabb.max[1],   8.0f);
    ASSERT_FLOAT_EQ(aabb.max[2],   7.0f);
}

// ---------------------------------------------------------------------------
// 4. frustumTestAABB: AABB at origin with identity world is visible
// ---------------------------------------------------------------------------

void test_frustum_aabb_at_origin_visible(void)
{
    mat4 vp;
    make_test_view_proj(vp);
    FrustumPlanes frustum;
    frustumExtractPlanes(vp, &frustum);

    AABB aabb = {{-1.0f, -1.0f, -1.0f}, {1.0f, 1.0f, 1.0f}};
    mat4 world;
    glm_mat4_identity(world);

    ASSERT_EQ(frustumTestAABB(&frustum, &aabb, world), 1);
}

// ---------------------------------------------------------------------------
// 5. frustumTestAABB: AABB translated behind the camera is culled
// ---------------------------------------------------------------------------

void test_frustum_aabb_behind_camera_culled(void)
{
    mat4 vp;
    make_test_view_proj(vp);
    FrustumPlanes frustum;
    frustumExtractPlanes(vp, &frustum);

    // Camera at (0,0,5) looking at origin (toward -z). z=20 is behind it.
    AABB aabb = {{-1.0f, -1.0f, -1.0f}, {1.0f, 1.0f, 1.0f}};
    mat4 world;
    translate_world(world, 0.0f, 0.0f, 20.0f);

    ASSERT_EQ(frustumTestAABB(&frustum, &aabb, world), 0);
}

// ---------------------------------------------------------------------------
// 6. frustumTestAABB: AABB far off to the side is culled
// ---------------------------------------------------------------------------

void test_frustum_aabb_far_to_side_culled(void)
{
    mat4 vp;
    make_test_view_proj(vp);
    FrustumPlanes frustum;
    frustumExtractPlanes(vp, &frustum);

    AABB aabb = {{-1.0f, -1.0f, -1.0f}, {1.0f, 1.0f, 1.0f}};
    mat4 world;
    translate_world(world, 1000.0f, 0.0f, 0.0f);

    ASSERT_EQ(frustumTestAABB(&frustum, &aabb, world), 0);
}

// ---------------------------------------------------------------------------
// 7. frustumTestAABB: AABB beyond the far plane is culled
// ---------------------------------------------------------------------------

void test_frustum_aabb_beyond_far_plane_culled(void)
{
    mat4 vp;
    make_test_view_proj(vp);
    FrustumPlanes frustum;
    frustumExtractPlanes(vp, &frustum);

    // Camera at z=5, far=100, so far plane is at z = 5 - 100 = -95.
    AABB aabb = {{-1.0f, -1.0f, -1.0f}, {1.0f, 1.0f, 1.0f}};
    mat4 world;
    translate_world(world, 0.0f, 0.0f, -200.0f);

    ASSERT_EQ(frustumTestAABB(&frustum, &aabb, world), 0);
}

// ---------------------------------------------------------------------------
// 8. frustumTestAABB: Arvo applies world translation correctly
//    (AABB in local space lives at +1000 on x; world matrix shifts -1000)
// ---------------------------------------------------------------------------

void test_frustum_arvo_world_translation_back_into_view(void)
{
    mat4 vp;
    make_test_view_proj(vp);
    FrustumPlanes frustum;
    frustumExtractPlanes(vp, &frustum);

    AABB aabb = {{999.0f, -1.0f, -1.0f}, {1001.0f, 1.0f, 1.0f}};
    mat4 world;
    translate_world(world, -1000.0f, 0.0f, 0.0f);

    // After Arvo transform, world AABB sits at the origin → visible.
    ASSERT_EQ(frustumTestAABB(&frustum, &aabb, world), 1);
}

// ---------------------------------------------------------------------------
// 9. frustumCullScene: visible instances stay flagged 0
// ---------------------------------------------------------------------------

void test_cull_keeps_visible_instances(void)
{
    SCENE* scene = make_test_scene(1, 3);
    set_unit_cube_aabb_on_models(scene);
    TransformManager* tman = &scene->transformManager;

    transformSetPosition(&tman->pTransformsCold[0], (vec3){ 0.0f, 0.0f, 0.0f});
    transformSetPosition(&tman->pTransformsCold[1], (vec3){ 2.0f, 0.0f, 0.0f});
    transformSetPosition(&tman->pTransformsCold[2], (vec3){-2.0f, 0.0f, 0.0f});

    resolveTransforms(tman);

    mat4 vp;
    make_test_view_proj(vp);
    FrustumPlanes frustum;
    frustumExtractPlanes(vp, &frustum);

    CAPTURE_STDERR_BEGIN();
    frustumCullScene(scene, &frustum);
    CAPTURE_STDERR_END();

    ASSERT_EQ(tman->pTransformsHot[0].passFlags, 0);
    ASSERT_EQ(tman->pTransformsHot[1].passFlags, 0);
    ASSERT_EQ(tman->pTransformsHot[2].passFlags, 0);

    free_test_scene(scene);
}

// ---------------------------------------------------------------------------
// 10. frustumCullScene: off-screen instances get TRANSFORM_PASS_DISABLED
// ---------------------------------------------------------------------------

void test_cull_marks_offscreen_instances(void)
{
    SCENE* scene = make_test_scene(1, 3);
    set_unit_cube_aabb_on_models(scene);
    TransformManager* tman = &scene->transformManager;

    transformSetPosition(&tman->pTransformsCold[0], (vec3){    0.0f, 0.0f,   0.0f}); // visible
    transformSetPosition(&tman->pTransformsCold[1], (vec3){ 1000.0f, 0.0f,   0.0f}); // off to side
    transformSetPosition(&tman->pTransformsCold[2], (vec3){    0.0f, 0.0f, 200.0f}); // behind camera

    resolveTransforms(tman);

    mat4 vp;
    make_test_view_proj(vp);
    FrustumPlanes frustum;
    frustumExtractPlanes(vp, &frustum);

    CAPTURE_STDERR_BEGIN();
    frustumCullScene(scene, &frustum);
    CAPTURE_STDERR_END();

    ASSERT_EQ(tman->pTransformsHot[0].passFlags, 0);
    ASSERT_EQ(tman->pTransformsHot[1].passFlags, TRANSFORM_PASS_DISABLED);
    ASSERT_EQ(tman->pTransformsHot[2].passFlags, TRANSFORM_PASS_DISABLED);

    free_test_scene(scene);
}

// ---------------------------------------------------------------------------
// 11. frustumCullScene: respects existing per-frame disable, doesn't touch
//     destroyed instances
// ---------------------------------------------------------------------------

void test_cull_skips_destroyed_and_already_disabled(void)
{
    SCENE* scene = make_test_scene(1, 3);
    set_unit_cube_aabb_on_models(scene);
    TransformManager* tman = &scene->transformManager;

    // All three sit at the origin — would be visible
    transformSetPosition(&tman->pTransformsCold[0], (vec3){0.0f, 0.0f, 0.0f});
    transformSetPosition(&tman->pTransformsCold[1], (vec3){0.0f, 0.0f, 0.0f});
    transformSetPosition(&tman->pTransformsCold[2], (vec3){0.0f, 0.0f, 0.0f});

    // Instance 1: destroyed at the instance level (cull short-circuits before reading transform)
    scene->modelsManager.pDenseModelInstances[1].lifecycle = INSTANCE_DESTROYED;
    // Transform 2: lifecycle-disabled (resolve will set passFlags = DISABLED)
    tman->pTransformsHot[2].lifecycle = TRANSFORM_DISABLED;

    resolveTransforms(tman);

    mat4 vp;
    make_test_view_proj(vp);
    FrustumPlanes frustum;
    frustumExtractPlanes(vp, &frustum);

    CAPTURE_STDERR_BEGIN();
    frustumCullScene(scene, &frustum);
    CAPTURE_STDERR_END();

    // Alive + visible → not flagged
    ASSERT_EQ(tman->pTransformsHot[0].passFlags, 0);
    // Destroyed instance: cull continued before touching the transform, so passFlags stays 0
    ASSERT_EQ(tman->pTransformsHot[1].passFlags, 0);
    // Lifecycle-disabled transform: resolve set DISABLED, cull leaves it alone
    ASSERT_EQ(tman->pTransformsHot[2].passFlags, TRANSFORM_PASS_DISABLED);

    free_test_scene(scene);
}

// ---------------------------------------------------------------------------
// 12. frustumCullScene: lights are culled alongside models in a mixed scene
// ---------------------------------------------------------------------------

void test_cull_lights_alongside_models(void)
{
    SCENE* scene = make_test_scene_mixed(1, 2, 1, 2);
    set_unit_cube_aabb_on_models(scene);
    set_unit_cube_aabb_on_lights(scene);
    TransformManager* tman = &scene->transformManager;

    // model instances live at transform indices [0,1], lights at [2,3]
    transformSetPosition(&tman->pTransformsCold[0], (vec3){    0.0f, 0.0f,   0.0f});
    transformSetPosition(&tman->pTransformsCold[1], (vec3){ 1000.0f, 0.0f,   0.0f});
    transformSetPosition(&tman->pTransformsCold[2], (vec3){    0.0f, 0.0f,   0.0f});
    transformSetPosition(&tman->pTransformsCold[3], (vec3){    0.0f, 0.0f, 200.0f});

    resolveTransforms(tman);

    mat4 vp;
    make_test_view_proj(vp);
    FrustumPlanes frustum;
    frustumExtractPlanes(vp, &frustum);

    CAPTURE_STDERR_BEGIN();
    frustumCullScene(scene, &frustum);
    CAPTURE_STDERR_END();

    ASSERT_EQ(tman->pTransformsHot[0].passFlags, 0);
    ASSERT_EQ(tman->pTransformsHot[1].passFlags, TRANSFORM_PASS_DISABLED);
    ASSERT_EQ(tman->pTransformsHot[2].passFlags, 0);
    ASSERT_EQ(tman->pTransformsHot[3].passFlags, TRANSFORM_PASS_DISABLED);

    free_test_scene_mixed(scene);
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main(void)
{
    printf("=== Frustum Culling Tests ===\n");

    RUN_TEST(test_aabb_single_vertex);
    RUN_TEST(test_aabb_unit_cube);
    RUN_TEST(test_aabb_skewed_extents);
    RUN_TEST(test_aabb_postcondition_finite_and_ordered);

    RUN_TEST(test_frustum_aabb_at_origin_visible);
    RUN_TEST(test_frustum_aabb_behind_camera_culled);
    RUN_TEST(test_frustum_aabb_far_to_side_culled);
    RUN_TEST(test_frustum_aabb_beyond_far_plane_culled);
    RUN_TEST(test_frustum_arvo_world_translation_back_into_view);

    RUN_TEST(test_cull_keeps_visible_instances);
    RUN_TEST(test_cull_marks_offscreen_instances);
    RUN_TEST(test_cull_skips_destroyed_and_already_disabled);
    RUN_TEST(test_cull_lights_alongside_models);

    TEST_SUMMARY();
}
