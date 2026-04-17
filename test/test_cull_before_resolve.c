#include "test_harness.h"
#include "test_helpers.h"
#include "frustum.h"
#include "scene.h"
#include "sceneSpec.h"
#include "transform.h"
#include "transformNode.h"
#include "vertex.h"
#include "cglm.h"
#include <stdlib.h>
#include <string.h>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static void make_test_view_proj(mat4 out)
{
    mat4 view, proj;
    glm_lookat((vec3){0.0f, 0.0f, 5.0f}, (vec3){0.0f, 0.0f, 0.0f},
               (vec3){0.0f, 1.0f, 0.0f}, view);
    glm_perspective(glm_rad(45.0f), 1.0f, 0.1f, 100.0f, proj);
    proj[1][1] *= -1.0f;
    glm_mat4_mul(proj, view, out);
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
    TransformManager* tman = &scene->transformManager;
    for (uint32_t i = 0; i < tman->count; i++)
    {
        tman->pLocalAABBs[i].min[0] = -1.0f;
        tman->pLocalAABBs[i].min[1] = -1.0f;
        tman->pLocalAABBs[i].min[2] = -1.0f;
        tman->pLocalAABBs[i].max[0] =  1.0f;
        tman->pLocalAABBs[i].max[1] =  1.0f;
        tman->pLocalAABBs[i].max[2] =  1.0f;
    }
}

static void run_cull(SCENE* scene)
{
    mat4 vp;
    make_test_view_proj(vp);
    FrustumPlanes frustum;
    frustumExtractPlanes(vp, &frustum);
    CAPTURE_STDERR_BEGIN();
    frustumCullSceneSphere(scene, &frustum);
    CAPTURE_STDERR_END();
}

// ---------------------------------------------------------------------------
// 1. Cull's dirty-override: an offscreen node marked DIRTY must NOT be left
//    with FRUSTUM_CULLED set — its last-frame worldMatrix is stale and a
//    frustum verdict based on it would lie.
// ---------------------------------------------------------------------------

void test_cull_never_sets_frustum_culled_on_dirty(void)
{
    SCENE* scene = make_test_scene(1, 1);
    set_unit_cube_aabb_on_models(scene);
    TransformManager* tman = &scene->transformManager;

    // Put the node firmly offscreen (behind the camera at z=5 looking toward z=0).
    transformSetPosition(&tman->pTransformsCold[0], (vec3){0.0f, 0.0f, 500.0f});
    resolveTransforms(tman);

    // Re-dirty without resolving, so when cull runs it sees DIRTY + stale
    // worldMatrix. Dirty-override must clear FRUSTUM_CULLED.
    tman->pTransformsHot[0].passFlags |= TRANSFORM_PASS_DIRTY;
    tman->pTransformsHot[0].passFlags |= TRANSFORM_PASS_FRUSTUM_CULLED; // stale from previous frame

    run_cull(scene);

    ASSERT_EQ(tman->pTransformsHot[0].passFlags & TRANSFORM_PASS_FRUSTUM_CULLED, 0);
    ASSERT_EQ(tman->pTransformsHot[0].passFlags & TRANSFORM_PASS_DIRTY, TRANSFORM_PASS_DIRTY);

    free_test_scene(scene);
}

// ---------------------------------------------------------------------------
// 2. Resolve's skip-mask: a node entering resolve with FRUSTUM_CULLED and
//    DIRTY set (possible only when g_visibilityCacheInvalidated == 0 and the
//    flags were stamped out-of-band) must be skipped, and DIRTY must be
//    preserved across the resolve.
// ---------------------------------------------------------------------------

void test_resolve_skips_frustum_culled_preserving_dirty(void)
{
    SCENE* scene = make_test_scene(1, 1);
    set_unit_cube_aabb_on_models(scene);
    TransformManager* tman = &scene->transformManager;

    transformSetPosition(&tman->pTransformsCold[0], (vec3){0.0f, 0.0f, 0.0f});
    resolveTransforms(tman);
    g_visibilityCacheInvalidated = 0; // explicit: we want the normal path

    // Snapshot the resolved matrix.
    mat4 before;
    glm_mat4_copy(tman->pTransformsWorld[0].worldMatrix, before);

    // Move the cold TRS so resolve *would* produce a different matrix if it ran.
    transformSetPosition(&tman->pTransformsCold[0], (vec3){42.0f, 0.0f, 0.0f});

    // Force the pre-cull state we want to test.
    tman->pTransformsHot[0].passFlags |= TRANSFORM_PASS_FRUSTUM_CULLED;
    tman->pTransformsHot[0].passFlags |= TRANSFORM_PASS_DIRTY;

    resolveTransforms(tman);

    // Resolve skipped: worldMatrix unchanged, DIRTY still set, FRUSTUM_CULLED still set.
    ASSERT_FLOAT_EQ(tman->pTransformsWorld[0].worldMatrix[3][0], before[3][0]);
    ASSERT_EQ(tman->pTransformsHot[0].passFlags & TRANSFORM_PASS_DIRTY, TRANSFORM_PASS_DIRTY);
    ASSERT_EQ(tman->pTransformsHot[0].passFlags & TRANSFORM_PASS_FRUSTUM_CULLED, TRANSFORM_PASS_FRUSTUM_CULLED);

    free_test_scene(scene);
}

// ---------------------------------------------------------------------------
// 3. g_visibilityCacheInvalidated bypasses FRUSTUM_CULLED for exactly one
//    frame. After resolve consumes it, the flag returns to 0.
// ---------------------------------------------------------------------------

void test_visibility_cache_invalidate_forces_resolve(void)
{
    SCENE* scene = make_test_scene(1, 1);
    set_unit_cube_aabb_on_models(scene);
    TransformManager* tman = &scene->transformManager;

    transformSetPosition(&tman->pTransformsCold[0], (vec3){0.0f, 0.0f, 0.0f});
    resolveTransforms(tman);

    // Move position, mark DIRTY + FRUSTUM_CULLED simultaneously (simulating a
    // camera cut: mesh moved but was culled last frame before cut).
    transformSetPosition(&tman->pTransformsCold[0], (vec3){7.0f, 0.0f, 0.0f});
    tman->pTransformsHot[0].passFlags |= TRANSFORM_PASS_FRUSTUM_CULLED;
    tman->pTransformsHot[0].passFlags |= TRANSFORM_PASS_DIRTY;

    g_visibilityCacheInvalidated = 1;
    resolveTransforms(tman);

    // Resolve ran: worldMatrix updated to new position, FRUSTUM_CULLED cleared,
    // DIRTY cleared (pass 2 clears it on resolved nodes), cache flag consumed.
    ASSERT_FLOAT_EQ(tman->pTransformsWorld[0].worldMatrix[3][0], 7.0f);
    ASSERT_EQ(tman->pTransformsHot[0].passFlags & TRANSFORM_PASS_FRUSTUM_CULLED, 0);
    ASSERT_EQ(tman->pTransformsHot[0].passFlags & TRANSFORM_PASS_DIRTY, 0);
    ASSERT_EQ(g_visibilityCacheInvalidated, 0);

    free_test_scene(scene);
}

// ---------------------------------------------------------------------------
// 4. markSubtreeCulled skips DIRTY descendants. A parent-outside sphere must
//    propagate FRUSTUM_CULLED through the subtree, except children already
//    flagged DIRTY — their stale worldMatrix precludes a trustworthy cull
//    decision, so they remain un-flagged and fall through to resolve.
// ---------------------------------------------------------------------------

static SCENE* make_hier_scene_one_parent_two_children(void)
{
    // Mirrors test_spec_tree.c's pattern: calloc scene + one model type +
    // build via sceneSpec. fillSceneFromSpecTree handles all transform arrays.
    SCENE* scene = (SCENE*) calloc(1, sizeof(SCENE));
    scene->modelsManager.pModel   = (Model3D*) calloc(1, sizeof(Model3D));
    scene->modelsManager.modelLen = 1;

    GEOM_VERTEX_COLOR_RGBA white = {1.0f, 1.0f, 1.0f, 1.0f};
    SceneEntitySpec* root = sceneSpecNewModel(0, (vec3){1000.0f, 0.0f, 0.0f}, white);
    SceneEntitySpec* kidA = sceneSpecNewModel(0, (vec3){0.0f,    0.0f, 0.0f}, white);
    SceneEntitySpec* kidB = sceneSpecNewModel(0, (vec3){0.5f,    0.0f, 0.0f}, white);
    sceneSpecAddChild(root, kidA);
    sceneSpecAddChild(root, kidB);
    fillSceneFromSpecTree(scene, &root, 1);
    sceneSpecFree(root);
    return scene;
}

static void free_hier_scene(SCENE* scene)
{
    if (!scene) return;
    free(scene->modelsManager.pModel);
    free(scene->modelsManager.pDenseModelInstances);
    free(scene->modelsManager.instanceOffset.items);
    free(scene->lightManager.pLight);
    free(scene->lightManager.pDenseLightInstances);
    free(scene->lightManager.instanceOffset.items);
    free(scene->transformManager.pTransformsHot);
    free(scene->transformManager.pTransformsWorld);
    free(scene->transformManager.pTransformsCold);
    free(scene->transformManager.pPreorderEntities);
    free(scene->transformManager.pLocalAABBs);
    free(scene);
}

void test_marksubtreeculled_skips_dirty_descendants(void)
{
    SCENE* scene = make_hier_scene_one_parent_two_children();
    set_unit_cube_aabb_on_models(scene);
    TransformManager* tman = &scene->transformManager;

    resolveTransforms(tman);

    // Child at index 1 is manually re-dirtied between resolve and cull.
    // Child at index 2 stays clean.
    tman->pTransformsHot[1].passFlags |= TRANSFORM_PASS_DIRTY;

    run_cull(scene);

    // Parent: outside frustum (world x=1000) → FRUSTUM_CULLED set.
    // Child 1: DIRTY before cull → FRUSTUM_CULLED must NOT be set.
    // Child 2: clean → FRUSTUM_CULLED propagated from parent.
    ASSERT_EQ(tman->pTransformsHot[0].passFlags & TRANSFORM_PASS_FRUSTUM_CULLED, TRANSFORM_PASS_FRUSTUM_CULLED);
    ASSERT_EQ(tman->pTransformsHot[1].passFlags & TRANSFORM_PASS_FRUSTUM_CULLED, 0);
    ASSERT_EQ(tman->pTransformsHot[1].passFlags & TRANSFORM_PASS_DIRTY, TRANSFORM_PASS_DIRTY);
    ASSERT_EQ(tman->pTransformsHot[2].passFlags & TRANSFORM_PASS_FRUSTUM_CULLED, TRANSFORM_PASS_FRUSTUM_CULLED);

    free_hier_scene(scene);
}

// ---------------------------------------------------------------------------
// 5. Mixed scene: clean-visible, clean-invisible, dirty-visible, dirty-invisible.
//    Exercise all four cull+resolve cells in a single sweep.
// ---------------------------------------------------------------------------

void test_mixed_scene_four_cells(void)
{
    SCENE* scene = make_test_scene(1, 4);
    set_unit_cube_aabb_on_models(scene);
    TransformManager* tman = &scene->transformManager;

    // 0: clean + visible   → cull clears FRUSTUM_CULLED, resolve skips dirty branch (clean, no parent-dirty).
    // 1: clean + invisible → cull sets FRUSTUM_CULLED, resolve skips.
    // 2: dirty + visible   → cull clears FRUSTUM_CULLED, resolve recomputes.
    // 3: dirty + invisible → cull's dirty-override: FRUSTUM_CULLED stays clear, resolve recomputes.
    transformSetPosition(&tman->pTransformsCold[0], (vec3){0.0f, 0.0f, 0.0f});
    transformSetPosition(&tman->pTransformsCold[1], (vec3){500.0f, 0.0f, 0.0f});
    transformSetPosition(&tman->pTransformsCold[2], (vec3){0.5f, 0.0f, 0.0f});
    transformSetPosition(&tman->pTransformsCold[3], (vec3){-500.0f, 0.0f, 0.0f});
    resolveTransforms(tman);
    g_visibilityCacheInvalidated = 0;

    // Move 2 and 3 and mark them dirty (their cached worldMatrix is stale).
    transformSetPosition(&tman->pTransformsCold[2], (vec3){1.0f, 0.0f, 0.0f});
    transformSetPosition(&tman->pTransformsCold[3], (vec3){-501.0f, 0.0f, 0.0f});
    tman->pTransformsHot[2].passFlags |= TRANSFORM_PASS_DIRTY;
    tman->pTransformsHot[3].passFlags |= TRANSFORM_PASS_DIRTY;

    run_cull(scene);

    ASSERT_EQ(tman->pTransformsHot[0].passFlags & TRANSFORM_PASS_FRUSTUM_CULLED, 0);
    ASSERT_EQ(tman->pTransformsHot[1].passFlags & TRANSFORM_PASS_FRUSTUM_CULLED, TRANSFORM_PASS_FRUSTUM_CULLED);
    ASSERT_EQ(tman->pTransformsHot[2].passFlags & TRANSFORM_PASS_FRUSTUM_CULLED, 0);
    ASSERT_EQ(tman->pTransformsHot[3].passFlags & TRANSFORM_PASS_FRUSTUM_CULLED, 0);

    resolveTransforms(tman);

    // 0: clean visible — matrix unchanged.
    ASSERT_FLOAT_EQ(tman->pTransformsWorld[0].worldMatrix[3][0], 0.0f);
    // 1: clean invisible — matrix unchanged, DIRTY sticky (stayed set).
    ASSERT_FLOAT_EQ(tman->pTransformsWorld[1].worldMatrix[3][0], 500.0f);
    ASSERT_EQ(tman->pTransformsHot[1].passFlags & TRANSFORM_PASS_FRUSTUM_CULLED, TRANSFORM_PASS_FRUSTUM_CULLED);
    // 2: dirty visible — matrix updated to 1.0.
    ASSERT_FLOAT_EQ(tman->pTransformsWorld[2].worldMatrix[3][0], 1.0f);
    ASSERT_EQ(tman->pTransformsHot[2].passFlags & TRANSFORM_PASS_DIRTY, 0);
    // 3: dirty invisible — dirty-override forced resolve; matrix updated to -501.
    ASSERT_FLOAT_EQ(tman->pTransformsWorld[3].worldMatrix[3][0], -501.0f);
    ASSERT_EQ(tman->pTransformsHot[3].passFlags & TRANSFORM_PASS_DIRTY, 0);

    free_test_scene(scene);
}

// ---------------------------------------------------------------------------

int main(void)
{
    printf("=== Cull-Before-Resolve Tests ===\n");
    RUN_TEST(test_cull_never_sets_frustum_culled_on_dirty);
    RUN_TEST(test_resolve_skips_frustum_culled_preserving_dirty);
    RUN_TEST(test_visibility_cache_invalidate_forces_resolve);
    RUN_TEST(test_marksubtreeculled_skips_dirty_descendants);
    RUN_TEST(test_mixed_scene_four_cells);
    TEST_SUMMARY();
}
