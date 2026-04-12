#include "test_harness.h"
#include "sceneSpec.h"
#include "scene.h"
#include "model.h"
#include "light.h"
#include "transformNode.h"
#include "frustum.h"
#include "vertex.h"
#include "cglm.h"
#include <stdlib.h>
#include <string.h>

// ---------------------------------------------------------------------------
// Cross-type bidirectional parenting tests.
//
// The preorder refactor (plan_preorder_hierarchy.md Phase 2) enabled
// parenting in both directions:
//
//   (A) model under light  — the common case, covered by test_spec_tree.c::
//       test_cross_type_model_under_light
//
//   (B) light under model  — symmetric, supported by the API but NEVER
//       actually exercised by a test until now.
//
// This file covers (B) explicitly and adds numeric verification of
// `resolveTransforms` world-matrix composition for mixed-type chains.
//
// Verification strategy
// ---------------------
// Every transform in these tests uses identity rotation and unit scale,
// so each localMatrix is a pure translation and the composed worldMatrix is
// a pure translation equal to the sum of positions along the ancestor
// chain. The translation component lives in column 3 of the mat4:
//
//     worldMatrix[3][0..2] = Σ positions from root to self.
//
// Asserting that sum lets us prove that T_parent * T_child composition
// works correctly regardless of which type is at which depth.
// ---------------------------------------------------------------------------

static SCENE* make_empty_scene_with_types(uint32_t modelTypeCount, uint32_t lightTypeCount)
{
    SCENE* scene = (SCENE*) calloc(1, sizeof(SCENE));

    scene->modelsManager.pModel    = (Model3D*) calloc(modelTypeCount, sizeof(Model3D));
    scene->modelsManager.modelLen  = modelTypeCount;
    scene->lightManager.pLight     = (LightVolume*) calloc(lightTypeCount, sizeof(LightVolume));
    scene->lightManager.lightLen   = lightTypeCount;

    for (uint32_t m = 0; m < modelTypeCount; m++)
    {
        float extent = (float)(m + 1);
        scene->modelsManager.pModel[m].localAABB.min[0] = -extent;
        scene->modelsManager.pModel[m].localAABB.min[1] = -extent;
        scene->modelsManager.pModel[m].localAABB.min[2] = -extent;
        scene->modelsManager.pModel[m].localAABB.max[0] =  extent;
        scene->modelsManager.pModel[m].localAABB.max[1] =  extent;
        scene->modelsManager.pModel[m].localAABB.max[2] =  extent;
    }
    for (uint32_t l = 0; l < lightTypeCount; l++)
    {
        float extent = (float)(l + 10);
        scene->lightManager.pLight[l].localAABB.min[0] = -extent;
        scene->lightManager.pLight[l].localAABB.min[1] = -extent;
        scene->lightManager.pLight[l].localAABB.min[2] = -extent;
        scene->lightManager.pLight[l].localAABB.max[0] =  extent;
        scene->lightManager.pLight[l].localAABB.max[1] =  extent;
        scene->lightManager.pLight[l].localAABB.max[2] =  extent;
    }

    return scene;
}

static void free_empty_scene(SCENE* scene)
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

static GEOM_VERTEX_COLOR_RGBA white(void)
{
    return (GEOM_VERTEX_COLOR_RGBA){1.0f, 1.0f, 1.0f, 1.0f};
}

// Assert that a transform's worldMatrix represents a pure translation to
// (x, y, z). Tolerates the standard ASSERT_FLOAT_EQ epsilon of 1e-6 per
// component.
static void assert_world_translation(const TransformManager* tman, uint32_t idx,
                                     float x, float y, float z)
{
    const float* m = (const float*) tman->pTransformsWorld[idx].worldMatrix;
    // cglm stores mat4 column-major, so the translation column is elements 12..14.
    ASSERT_FLOAT_EQ(m[12], x);
    ASSERT_FLOAT_EQ(m[13], y);
    ASSERT_FLOAT_EQ(m[14], z);
}

// ---------------------------------------------------------------------------
// 1. Light under model — THE missing direction.
//
//    Tree:
//         model (pos 1,0,0)
//           |
//         light (pos 0,2,0)
//
//    Expected after resolve:
//      model.world = (1,0,0)
//      light.world = (1,2,0)   — composed through the model parent
// ---------------------------------------------------------------------------

void test_light_under_model_structure_and_world(void)
{
    SCENE* scene = make_empty_scene_with_types(1, 1);

    SceneEntitySpec* modelRoot  = sceneSpecNewModel(0, (vec3){1.0f, 0.0f, 0.0f}, white());
    SceneEntitySpec* lightChild = sceneSpecNewLight(0, (vec3){0.0f, 2.0f, 0.0f}, 1.0f, white());
    sceneSpecAddChild(modelRoot, lightChild);

    fillSceneFromSpecTree(scene, &modelRoot, 1);

    // Structural: two nodes in preorder, model root first, light child second.
    ASSERT_EQ(scene->transformManager.count, 2);
    ASSERT_EQ(scene->modelsManager.modelInstanceCapacity, 1);
    ASSERT_EQ(scene->lightManager.lightInstanceCapacity, 1);

    ASSERT_EQ(scene->transformManager.pTransformsHot[0].parentIdx, TRANSFORM_NO_PARENT);
    ASSERT_EQ(scene->transformManager.pTransformsHot[0].skipCount, 1);
    ASSERT_EQ(scene->transformManager.pPreorderEntities[0].kind, (uint16_t) SCENE_ENTITY_MODEL);
    ASSERT_EQ(scene->transformManager.pPreorderEntities[0].denseIdx, 0);

    ASSERT_EQ(scene->transformManager.pTransformsHot[1].parentIdx, 0);
    ASSERT_EQ(scene->transformManager.pTransformsHot[1].skipCount, 0);
    ASSERT_EQ(scene->transformManager.pPreorderEntities[1].kind, (uint16_t) SCENE_ENTITY_LIGHT);
    ASSERT_EQ(scene->transformManager.pPreorderEntities[1].denseIdx, 0);

    // Back-references.
    ASSERT_EQ(scene->modelsManager.pDenseModelInstances[0].transformIdx, 0);
    ASSERT_EQ(scene->lightManager.pDenseLightInstances[0].transformIdx,  1);

    // AABBs: preorder 0 is model type 0 (extent 1), preorder 1 is light type 0 (extent 10).
    ASSERT_FLOAT_EQ(scene->transformManager.pLocalAABBs[0].max[0],  1.0f);
    ASSERT_FLOAT_EQ(scene->transformManager.pLocalAABBs[1].max[0], 10.0f);

    // World composition after resolve.
    resolveTransforms(&scene->transformManager);

    assert_world_translation(&scene->transformManager, 0, 1.0f, 0.0f, 0.0f);
    assert_world_translation(&scene->transformManager, 1, 1.0f, 2.0f, 0.0f);

    sceneSpecFree(modelRoot);
    free_empty_scene(scene);
}

// ---------------------------------------------------------------------------
// 2. Model under light — sanity check for the already-supported direction.
//    Mirror of test 1 with types swapped. Verifies world matrix composition
//    specifically, which test_spec_tree.c did not assert numerically.
// ---------------------------------------------------------------------------

void test_model_under_light_world_composition(void)
{
    SCENE* scene = make_empty_scene_with_types(1, 1);

    SceneEntitySpec* lightRoot  = sceneSpecNewLight(0, (vec3){3.0f, 0.0f, 0.0f}, 1.0f, white());
    SceneEntitySpec* modelChild = sceneSpecNewModel(0, (vec3){0.0f, 0.0f, 4.0f}, white());
    sceneSpecAddChild(lightRoot, modelChild);

    fillSceneFromSpecTree(scene, &lightRoot, 1);
    resolveTransforms(&scene->transformManager);

    // Light root at (3,0,0); model child at local (0,0,4) → world (3,0,4).
    assert_world_translation(&scene->transformManager, 0, 3.0f, 0.0f, 0.0f);
    assert_world_translation(&scene->transformManager, 1, 3.0f, 0.0f, 4.0f);

    sceneSpecFree(lightRoot);
    free_empty_scene(scene);
}

// ---------------------------------------------------------------------------
// 3. Deep alternating chain — L → M → L → M → L (5 levels). Ensures the
//    resolve loop propagates worldMatrix through every type boundary.
//
//    Positions along the chain:
//      L0  (1, 0, 0)
//      M0  (0, 1, 0)
//      L1  (0, 0, 1)
//      M1  (1, 1, 0)
//      L2  (0, 1, 1)
//
//    Expected cumulative world:
//      L0  (1, 0, 0)
//      M0  (1, 1, 0)
//      L1  (1, 1, 1)
//      M1  (2, 2, 1)
//      L2  (2, 3, 2)
// ---------------------------------------------------------------------------

void test_alternating_chain_depth5_world_composition(void)
{
    SCENE* scene = make_empty_scene_with_types(1, 1);

    SceneEntitySpec* l0 = sceneSpecNewLight(0, (vec3){1.0f, 0.0f, 0.0f}, 1.0f, white());
    SceneEntitySpec* m0 = sceneSpecNewModel(0, (vec3){0.0f, 1.0f, 0.0f}, white());
    SceneEntitySpec* l1 = sceneSpecNewLight(0, (vec3){0.0f, 0.0f, 1.0f}, 1.0f, white());
    SceneEntitySpec* m1 = sceneSpecNewModel(0, (vec3){1.0f, 1.0f, 0.0f}, white());
    SceneEntitySpec* l2 = sceneSpecNewLight(0, (vec3){0.0f, 1.0f, 1.0f}, 1.0f, white());
    sceneSpecAddChild(l0, m0);
    sceneSpecAddChild(m0, l1);
    sceneSpecAddChild(l1, m1);
    sceneSpecAddChild(m1, l2);

    fillSceneFromSpecTree(scene, &l0, 1);

    // Structure: preorder [L, M, L, M, L] with each node parented to the
    // previous one.
    ASSERT_EQ(scene->transformManager.count, 5);
    const SceneEntityKind expectedKinds[5] = {
        SCENE_ENTITY_LIGHT, SCENE_ENTITY_MODEL, SCENE_ENTITY_LIGHT, SCENE_ENTITY_MODEL, SCENE_ENTITY_LIGHT
    };
    for (uint32_t i = 0; i < 5; i++)
    {
        ASSERT_EQ(scene->transformManager.pPreorderEntities[i].kind, (uint16_t) expectedKinds[i]);
    }
    ASSERT_EQ(scene->transformManager.pTransformsHot[0].parentIdx, TRANSFORM_NO_PARENT);
    for (uint32_t i = 1; i < 5; i++)
    {
        ASSERT_EQ(scene->transformManager.pTransformsHot[i].parentIdx, i - 1);
    }

    resolveTransforms(&scene->transformManager);

    assert_world_translation(&scene->transformManager, 0, 1.0f, 0.0f, 0.0f);
    assert_world_translation(&scene->transformManager, 1, 1.0f, 1.0f, 0.0f);
    assert_world_translation(&scene->transformManager, 2, 1.0f, 1.0f, 1.0f);
    assert_world_translation(&scene->transformManager, 3, 2.0f, 2.0f, 1.0f);
    assert_world_translation(&scene->transformManager, 4, 2.0f, 3.0f, 2.0f);

    sceneSpecFree(l0);
    free_empty_scene(scene);
}

// ---------------------------------------------------------------------------
// 4. Mixed siblings — a model root with two siblings of different types.
//    Tree:
//              model root (5, 0, 0)
//               /          \
//        light child     model child
//        (0, 3, 0)       (0, 0, 7)
//
//    Expected worlds:
//      root         = (5, 0, 0)
//      light child  = (5, 3, 0)
//      model child  = (5, 0, 7)
//
//    Verifies that sibling world matrices are both composed from the same
//    parent regardless of their types.
// ---------------------------------------------------------------------------

void test_mixed_siblings_world_composition(void)
{
    SCENE* scene = make_empty_scene_with_types(1, 1);

    SceneEntitySpec* root        = sceneSpecNewModel(0, (vec3){5.0f, 0.0f, 0.0f}, white());
    SceneEntitySpec* lightChild  = sceneSpecNewLight(0, (vec3){0.0f, 3.0f, 0.0f}, 1.0f, white());
    SceneEntitySpec* modelChild  = sceneSpecNewModel(0, (vec3){0.0f, 0.0f, 7.0f}, white());
    sceneSpecAddChild(root, lightChild);
    sceneSpecAddChild(root, modelChild);

    fillSceneFromSpecTree(scene, &root, 1);
    resolveTransforms(&scene->transformManager);

    // Preorder insertion order: root first, then lightChild, then modelChild.
    ASSERT_EQ(scene->transformManager.count, 3);
    ASSERT_EQ(scene->transformManager.pPreorderEntities[0].kind, (uint16_t) SCENE_ENTITY_MODEL);
    ASSERT_EQ(scene->transformManager.pPreorderEntities[1].kind, (uint16_t) SCENE_ENTITY_LIGHT);
    ASSERT_EQ(scene->transformManager.pPreorderEntities[2].kind, (uint16_t) SCENE_ENTITY_MODEL);

    ASSERT_EQ(scene->transformManager.pTransformsHot[1].parentIdx, 0);
    ASSERT_EQ(scene->transformManager.pTransformsHot[2].parentIdx, 0);

    assert_world_translation(&scene->transformManager, 0, 5.0f, 0.0f, 0.0f);
    assert_world_translation(&scene->transformManager, 1, 5.0f, 3.0f, 0.0f);
    assert_world_translation(&scene->transformManager, 2, 5.0f, 0.0f, 7.0f);

    sceneSpecFree(root);
    free_empty_scene(scene);
}

// ---------------------------------------------------------------------------
// 5. Subtree dirty propagation across type boundaries.
//
//    Tree:
//        L0 (1, 0, 0)
//         |
//        M0 (0, 1, 0)
//         |
//        L1 (0, 0, 1)
//
//    Initial resolve produces L0=(1,0,0), M0=(1,1,0), L1=(1,1,1).
//    Then:
//      - Translate L0 by (+10, 0, 0) via transformTranslate on cold, then
//        transformMarkDirty(L0).
//      - Re-resolve.
//      - Expected: L0=(11,0,0), M0=(11,1,0), L1=(11,1,1) — the dirty flag
//        must propagate from the light root through the model intermediate
//        down to the light leaf, recomputing every descendant world matrix.
//
//    This is the test that catches any regression where dirty propagation
//    stops at a type boundary.
// ---------------------------------------------------------------------------

void test_dirty_propagation_across_type_boundary(void)
{
    SCENE* scene = make_empty_scene_with_types(1, 1);

    SceneEntitySpec* l0 = sceneSpecNewLight(0, (vec3){1.0f, 0.0f, 0.0f}, 1.0f, white());
    SceneEntitySpec* m0 = sceneSpecNewModel(0, (vec3){0.0f, 1.0f, 0.0f}, white());
    SceneEntitySpec* l1 = sceneSpecNewLight(0, (vec3){0.0f, 0.0f, 1.0f}, 1.0f, white());
    sceneSpecAddChild(l0, m0);
    sceneSpecAddChild(m0, l1);

    fillSceneFromSpecTree(scene, &l0, 1);
    resolveTransforms(&scene->transformManager);

    assert_world_translation(&scene->transformManager, 0, 1.0f, 0.0f, 0.0f);
    assert_world_translation(&scene->transformManager, 1, 1.0f, 1.0f, 0.0f);
    assert_world_translation(&scene->transformManager, 2, 1.0f, 1.0f, 1.0f);

    // Mutate the root: translate +10 on X, mark dirty, re-resolve.
    transformTranslate(&scene->transformManager.pTransformsCold[0], (vec3){10.0f, 0.0f, 0.0f});
    transformMarkDirty(&scene->transformManager, 0);

    resolveTransforms(&scene->transformManager);

    // Both descendants must have re-composed via the new root world.
    assert_world_translation(&scene->transformManager, 0, 11.0f, 0.0f, 0.0f);
    assert_world_translation(&scene->transformManager, 1, 11.0f, 1.0f, 0.0f);
    assert_world_translation(&scene->transformManager, 2, 11.0f, 1.0f, 1.0f);

    sceneSpecFree(l0);
    free_empty_scene(scene);
}

// ---------------------------------------------------------------------------
// 6. Cascade-disable across type boundaries. Disabling a model intermediate
//    must cascade to its light descendants (not just same-type descendants).
//
//    Tree:
//        L0
//         |
//        M0   ← set lifecycle = TRANSFORM_DISABLED
//         |
//        L1
//
//    After resolve:
//      L0.passFlags does NOT have DISABLED set (resolve cleared it: live root)
//      M0.passFlags has DISABLED (lifecycle-disabled itself)
//      L1.passFlags has DISABLED (cascade from M0)
// ---------------------------------------------------------------------------

void test_cascade_disable_crosses_type_boundary(void)
{
    SCENE* scene = make_empty_scene_with_types(1, 1);

    SceneEntitySpec* l0 = sceneSpecNewLight(0, (vec3){0.0f, 0.0f, 0.0f}, 1.0f, white());
    SceneEntitySpec* m0 = sceneSpecNewModel(0, (vec3){0.0f, 0.0f, 0.0f}, white());
    SceneEntitySpec* l1 = sceneSpecNewLight(0, (vec3){0.0f, 0.0f, 0.0f}, 1.0f, white());
    sceneSpecAddChild(l0, m0);
    sceneSpecAddChild(m0, l1);

    fillSceneFromSpecTree(scene, &l0, 1);

    // Disable the model intermediate, leaving the light root and light leaf alive.
    scene->transformManager.pTransformsHot[1].lifecycle = TRANSFORM_DISABLED;

    resolveTransforms(&scene->transformManager);

    ASSERT_EQ((scene->transformManager.pTransformsHot[0].passFlags & TRANSFORM_PASS_DISABLED), 0);
    ASSERT_EQ((scene->transformManager.pTransformsHot[1].passFlags & TRANSFORM_PASS_DISABLED), TRANSFORM_PASS_DISABLED);
    ASSERT_EQ((scene->transformManager.pTransformsHot[2].passFlags & TRANSFORM_PASS_DISABLED), TRANSFORM_PASS_DISABLED);

    sceneSpecFree(l0);
    free_empty_scene(scene);
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main(void)
{
    printf("=== Cross-Type Bidirectional Parenting Tests ===\n");

    RUN_TEST(test_light_under_model_structure_and_world);
    RUN_TEST(test_model_under_light_world_composition);
    RUN_TEST(test_alternating_chain_depth5_world_composition);
    RUN_TEST(test_mixed_siblings_world_composition);
    RUN_TEST(test_dirty_propagation_across_type_boundary);
    RUN_TEST(test_cascade_disable_crosses_type_boundary);

    TEST_SUMMARY();
}
