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
// Helpers — build a minimal SCENE with per-type metadata but no dense arrays.
// fillSceneFromSpecTree() will allocate the dense arrays.
// ---------------------------------------------------------------------------

static SCENE* make_empty_scene_with_types(uint32_t modelTypeCount, uint32_t lightTypeCount)
{
    SCENE* scene = (SCENE*) calloc(1, sizeof(SCENE));

    scene->modelsManager.pModel    = (Model3D*) calloc(modelTypeCount, sizeof(Model3D));
    scene->modelsManager.modelLen  = modelTypeCount;
    scene->lightManager.pLight     = (LightVolume*) calloc(lightTypeCount, sizeof(LightVolume));
    scene->lightManager.lightLen   = lightTypeCount;

    // Assign distinct per-type AABBs so we can verify the copy later.
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

// ---------------------------------------------------------------------------
// 1. Flat roots (no hierarchy): 3 model instances of type 0, no lights.
// ---------------------------------------------------------------------------

void test_flat_roots_three_models(void)
{
    SCENE* scene = make_empty_scene_with_types(1, 0);

    SceneEntitySpec* roots[3];
    roots[0] = sceneSpecNewModel(0, (vec3){1.0f, 0.0f, 0.0f}, white());
    roots[1] = sceneSpecNewModel(0, (vec3){2.0f, 0.0f, 0.0f}, white());
    roots[2] = sceneSpecNewModel(0, (vec3){3.0f, 0.0f, 0.0f}, white());

    fillSceneFromSpecTree(scene, roots, 3);

    // 3 entities total
    ASSERT_EQ(scene->transformManager.count, 3);
    ASSERT_EQ(scene->modelsManager.modelInstanceCapacity, 3);
    ASSERT_EQ(scene->modelsManager.pModel[0].instanceCount, 3);
    ASSERT_EQ(scene->modelsManager.pModel[0].denseBaseOffset, 0);
    ASSERT_EQ(scene->lightManager.lightInstanceCapacity, 0);

    // All three are roots (no parent), all skipCount = 0 (leaves)
    for (uint32_t i = 0; i < 3; i++)
    {
        ASSERT_EQ(scene->transformManager.pTransformsHot[i].parentIdx, TRANSFORM_NO_PARENT);
        ASSERT_EQ(scene->transformManager.pTransformsHot[i].skipCount, 0);
        ASSERT_EQ(scene->transformManager.pPreorderEntities[i].kind, (uint16_t) SCENE_ENTITY_MODEL);
        ASSERT_EQ(scene->transformManager.pPreorderEntities[i].typeIdx, 0);
    }

    // Dense model instances point back to their transforms
    for (uint32_t i = 0; i < 3; i++)
    {
        ASSERT_EQ(scene->modelsManager.pDenseModelInstances[i].transformIdx, i);
        ASSERT_EQ(scene->modelsManager.pDenseModelInstances[i].lifecycle, INSTANCE_ALIVE);
    }

    // Positions from spec are in cold slots
    ASSERT_FLOAT_EQ(scene->transformManager.pTransformsCold[0].position[0], 1.0f);
    ASSERT_FLOAT_EQ(scene->transformManager.pTransformsCold[1].position[0], 2.0f);
    ASSERT_FLOAT_EQ(scene->transformManager.pTransformsCold[2].position[0], 3.0f);

    // Local AABBs copied from type 0 (extent = 1.0)
    for (uint32_t i = 0; i < 3; i++)
    {
        ASSERT_FLOAT_EQ(scene->transformManager.pLocalAABBs[i].min[0], -1.0f);
        ASSERT_FLOAT_EQ(scene->transformManager.pLocalAABBs[i].max[0],  1.0f);
    }

    for (uint32_t i = 0; i < 3; i++) sceneSpecFree(roots[i]);
    free_empty_scene(scene);
}

// ---------------------------------------------------------------------------
// 2. Single root with two children: verify preorder layout + skipCount
//    Tree:
//         root (model 0)
//        /              \
//    child0 (model 0)  child1 (model 0)
//
//    Expected preorder: [root, child0, child1]
//    Expected skipCounts: [2, 0, 0]
// ---------------------------------------------------------------------------

void test_single_root_two_children(void)
{
    SCENE* scene = make_empty_scene_with_types(1, 0);

    SceneEntitySpec* root   = sceneSpecNewModel(0, (vec3){0.0f, 0.0f, 0.0f}, white());
    SceneEntitySpec* child0 = sceneSpecNewModel(0, (vec3){1.0f, 0.0f, 0.0f}, white());
    SceneEntitySpec* child1 = sceneSpecNewModel(0, (vec3){2.0f, 0.0f, 0.0f}, white());
    sceneSpecAddChild(root, child0);
    sceneSpecAddChild(root, child1);

    fillSceneFromSpecTree(scene, &root, 1);

    ASSERT_EQ(scene->transformManager.count, 3);

    // Preorder: [root=0, child0=1, child1=2]
    ASSERT_EQ(scene->transformManager.pTransformsHot[0].parentIdx, TRANSFORM_NO_PARENT);
    ASSERT_EQ(scene->transformManager.pTransformsHot[1].parentIdx, 0);
    ASSERT_EQ(scene->transformManager.pTransformsHot[2].parentIdx, 0);

    // skipCount reflects subtree size: root has 2 descendants, children have 0
    ASSERT_EQ(scene->transformManager.pTransformsHot[0].skipCount, 2);
    ASSERT_EQ(scene->transformManager.pTransformsHot[1].skipCount, 0);
    ASSERT_EQ(scene->transformManager.pTransformsHot[2].skipCount, 0);

    // Positions preserved in cold
    ASSERT_FLOAT_EQ(scene->transformManager.pTransformsCold[0].position[0], 0.0f);
    ASSERT_FLOAT_EQ(scene->transformManager.pTransformsCold[1].position[0], 1.0f);
    ASSERT_FLOAT_EQ(scene->transformManager.pTransformsCold[2].position[0], 2.0f);

    sceneSpecFree(root);
    free_empty_scene(scene);
}

// ---------------------------------------------------------------------------
// 3. Cross-type parenting: light at the root with a model child.
//    Tests that a model can be a descendant of a light (impossible under the
//    old "models first, lights second" allocation order).
//    Tree:
//         light (type 0)
//           |
//         model (type 0)
//
//    Expected preorder: [light=0, model=1]
//    Expected skipCounts: [1, 0]
//    light's denseIdx = 0 in pDenseLightInstances
//    model's denseIdx = 0 in pDenseModelInstances
// ---------------------------------------------------------------------------

void test_cross_type_model_under_light(void)
{
    SCENE* scene = make_empty_scene_with_types(1, 1);

    SceneEntitySpec* lightRoot  = sceneSpecNewLight(0, (vec3){5.0f, 0.0f, 0.0f}, 2.0f, white());
    SceneEntitySpec* modelChild = sceneSpecNewModel(0, (vec3){0.0f, 1.0f, 0.0f}, white());
    sceneSpecAddChild(lightRoot, modelChild);

    fillSceneFromSpecTree(scene, &lightRoot, 1);

    ASSERT_EQ(scene->transformManager.count, 2);
    ASSERT_EQ(scene->modelsManager.modelInstanceCapacity, 1);
    ASSERT_EQ(scene->lightManager.lightInstanceCapacity, 1);

    // Preorder position 0 is the light root
    ASSERT_EQ(scene->transformManager.pTransformsHot[0].parentIdx, TRANSFORM_NO_PARENT);
    ASSERT_EQ(scene->transformManager.pTransformsHot[0].skipCount, 1);
    ASSERT_EQ(scene->transformManager.pPreorderEntities[0].kind, (uint16_t) SCENE_ENTITY_LIGHT);
    ASSERT_EQ(scene->transformManager.pPreorderEntities[0].denseIdx, 0);

    // Preorder position 1 is the model child — parent is light at idx 0
    ASSERT_EQ(scene->transformManager.pTransformsHot[1].parentIdx, 0);
    ASSERT_EQ(scene->transformManager.pTransformsHot[1].skipCount, 0);
    ASSERT_EQ(scene->transformManager.pPreorderEntities[1].kind, (uint16_t) SCENE_ENTITY_MODEL);
    ASSERT_EQ(scene->transformManager.pPreorderEntities[1].denseIdx, 0);

    // Back-references from dense instances
    ASSERT_EQ(scene->lightManager.pDenseLightInstances[0].transformIdx, 0);
    ASSERT_EQ(scene->modelsManager.pDenseModelInstances[0].transformIdx, 1);

    // AABBs: preorder 0 gets light type 0 (extent 10), preorder 1 gets model type 0 (extent 1)
    ASSERT_FLOAT_EQ(scene->transformManager.pLocalAABBs[0].max[0], 10.0f);
    ASSERT_FLOAT_EQ(scene->transformManager.pLocalAABBs[1].max[0],  1.0f);

    sceneSpecFree(lightRoot);
    free_empty_scene(scene);
}

// ---------------------------------------------------------------------------
// 4. Mixed tree with multiple type indices and a three-level chain.
//    Tree:
//         root (model type 0)
//           |
//         mid (model type 1)
//          /  \
//      leaf0  leaf1  (both light type 0)
//
//    Preorder: [root=0, mid=1, leaf0=2, leaf1=3]
//    skipCounts: [3, 2, 0, 0]
// ---------------------------------------------------------------------------

void test_three_level_mixed_tree(void)
{
    SCENE* scene = make_empty_scene_with_types(2, 1);

    SceneEntitySpec* root  = sceneSpecNewModel(0, (vec3){0.0f, 0.0f, 0.0f}, white());
    SceneEntitySpec* mid   = sceneSpecNewModel(1, (vec3){1.0f, 0.0f, 0.0f}, white());
    SceneEntitySpec* leaf0 = sceneSpecNewLight(0, (vec3){2.0f, 0.0f, 0.0f}, 1.0f, white());
    SceneEntitySpec* leaf1 = sceneSpecNewLight(0, (vec3){3.0f, 0.0f, 0.0f}, 1.0f, white());
    sceneSpecAddChild(root, mid);
    sceneSpecAddChild(mid, leaf0);
    sceneSpecAddChild(mid, leaf1);

    fillSceneFromSpecTree(scene, &root, 1);

    ASSERT_EQ(scene->transformManager.count, 4);
    ASSERT_EQ(scene->modelsManager.modelInstanceCapacity, 2);
    ASSERT_EQ(scene->lightManager.lightInstanceCapacity, 2);

    // Per-type counts
    ASSERT_EQ(scene->modelsManager.pModel[0].instanceCount, 1);
    ASSERT_EQ(scene->modelsManager.pModel[1].instanceCount, 1);
    ASSERT_EQ(scene->lightManager.pLight[0].lightVolumeInstanceCount, 2);

    // Model type 0 starts at denseBase 0, type 1 at denseBase 1
    ASSERT_EQ(scene->modelsManager.pModel[0].denseBaseOffset, 0);
    ASSERT_EQ(scene->modelsManager.pModel[1].denseBaseOffset, 1);

    // Preorder parent chain
    ASSERT_EQ(scene->transformManager.pTransformsHot[0].parentIdx, TRANSFORM_NO_PARENT);
    ASSERT_EQ(scene->transformManager.pTransformsHot[1].parentIdx, 0);
    ASSERT_EQ(scene->transformManager.pTransformsHot[2].parentIdx, 1);
    ASSERT_EQ(scene->transformManager.pTransformsHot[3].parentIdx, 1);

    // skipCounts
    ASSERT_EQ(scene->transformManager.pTransformsHot[0].skipCount, 3);
    ASSERT_EQ(scene->transformManager.pTransformsHot[1].skipCount, 2);
    ASSERT_EQ(scene->transformManager.pTransformsHot[2].skipCount, 0);
    ASSERT_EQ(scene->transformManager.pTransformsHot[3].skipCount, 0);

    // Kinds in preorder positions: [MODEL, MODEL, LIGHT, LIGHT]
    ASSERT_EQ(scene->transformManager.pPreorderEntities[0].kind, (uint16_t) SCENE_ENTITY_MODEL);
    ASSERT_EQ(scene->transformManager.pPreorderEntities[1].kind, (uint16_t) SCENE_ENTITY_MODEL);
    ASSERT_EQ(scene->transformManager.pPreorderEntities[2].kind, (uint16_t) SCENE_ENTITY_LIGHT);
    ASSERT_EQ(scene->transformManager.pPreorderEntities[3].kind, (uint16_t) SCENE_ENTITY_LIGHT);

    // Type indices
    ASSERT_EQ(scene->transformManager.pPreorderEntities[0].typeIdx, 0);
    ASSERT_EQ(scene->transformManager.pPreorderEntities[1].typeIdx, 1);

    sceneSpecFree(root);
    free_empty_scene(scene);
}

// ---------------------------------------------------------------------------
// 5. Preorder invariant check on a larger tree:
//    every transformsHot[i].parentIdx is either NO_PARENT or < i, AND
//    the subtree rooted at i occupies exactly [i+1, i+1+skipCount).
// ---------------------------------------------------------------------------

static uint32_t build_balanced_tree(SceneEntitySpec* parent, uint32_t modelType, int depth)
{
    if (depth == 0) return 1;
    uint32_t total = 1;
    for (int c = 0; c < 2; c++)
    {
        SceneEntitySpec* child = sceneSpecNewModel(modelType, (vec3){0.0f, 0.0f, 0.0f}, white());
        sceneSpecAddChild(parent, child);
        total += build_balanced_tree(child, modelType, depth - 1);
    }
    return total;
}

static void verify_preorder_invariants(TransformManager* tman)
{
    // Check parent-before-child invariant
    for (uint32_t i = 0; i < tman->count; i++)
    {
        uint32_t p = tman->pTransformsHot[i].parentIdx;
        if (p != TRANSFORM_NO_PARENT)
        {
            ASSERT_EQ(p < i, 1);
        }
    }

    // Check subtree contiguity: every descendant of i lives in [i+1, i+1+skipCount)
    // Implemented by checking that for each i, all transforms in that range
    // have i as an ancestor (following parentIdx chains).
    for (uint32_t i = 0; i < tman->count; i++)
    {
        uint32_t skip = tman->pTransformsHot[i].skipCount;
        for (uint32_t j = i + 1; j < i + 1 + skip && j < tman->count; j++)
        {
            // Walk j's ancestor chain, must reach i
            uint32_t cur   = j;
            int      found = 0;
            while (cur != TRANSFORM_NO_PARENT)
            {
                cur = tman->pTransformsHot[cur].parentIdx;
                if (cur == i)
                {
                    found = 1;
                    break;
                }
            }
            if (!found)
            {
                printf("FAIL\n    subtree contiguity violated: transform %u is not a descendant of %u\n", j, i);
                tests_failed++;
                return;
            }
        }
    }
}

void test_balanced_tree_invariants(void)
{
    SCENE* scene = make_empty_scene_with_types(1, 0);

    SceneEntitySpec* root = sceneSpecNewModel(0, (vec3){0.0f, 0.0f, 0.0f}, white());
    uint32_t expectedTotal = build_balanced_tree(root, 0, 4); // depth 4 balanced binary = 31 nodes

    fillSceneFromSpecTree(scene, &root, 1);

    ASSERT_EQ(scene->transformManager.count, expectedTotal);
    ASSERT_EQ(scene->transformManager.pTransformsHot[0].skipCount, expectedTotal - 1);

    verify_preorder_invariants(&scene->transformManager);

    sceneSpecFree(root);
    free_empty_scene(scene);
}

// ---------------------------------------------------------------------------
// 6. Frustum cull propagates to children when parent is culled.
//    Build a tree: parent at (1000,0,0) [outside frustum], two model children
//    at local (0,0,0) and (0.5,0,0). Run resolve, run cull. Parent should be
//    culled and children's passFlags must be set via the O(skipCount)
//    propagation loop without needing their own frustum tests.
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

void test_cull_frustum_fail_propagates_to_subtree(void)
{
    SCENE* scene = make_empty_scene_with_types(1, 0);

    SceneEntitySpec* root   = sceneSpecNewModel(0, (vec3){1000.0f, 0.0f, 0.0f}, white());
    SceneEntitySpec* childA = sceneSpecNewModel(0, (vec3){0.0f,    0.0f, 0.0f}, white());
    SceneEntitySpec* childB = sceneSpecNewModel(0, (vec3){0.5f,    0.0f, 0.0f}, white());
    sceneSpecAddChild(root, childA);
    sceneSpecAddChild(root, childB);

    fillSceneFromSpecTree(scene, &root, 1);

    // Compute world matrices
    resolveTransforms(&scene->transformManager);

    // Build frustum and cull
    mat4 vp;
    make_test_view_proj(vp);
    FrustumPlanes frustum;
    frustumExtractPlanes(vp, &frustum);

    frustumCullScene(scene, &frustum);

    // All three should be marked frustum-culled: parent via frustum test, children
    // via the propagation loop (no frustum test on them).
    ASSERT_EQ(scene->transformManager.pTransformsHot[0].passFlags & TRANSFORM_PASS_FRUSTUM_CULLED, TRANSFORM_PASS_FRUSTUM_CULLED);
    ASSERT_EQ(scene->transformManager.pTransformsHot[1].passFlags & TRANSFORM_PASS_FRUSTUM_CULLED, TRANSFORM_PASS_FRUSTUM_CULLED);
    ASSERT_EQ(scene->transformManager.pTransformsHot[2].passFlags & TRANSFORM_PASS_FRUSTUM_CULLED, TRANSFORM_PASS_FRUSTUM_CULLED);

    sceneSpecFree(root);
    free_empty_scene(scene);
}

// ---------------------------------------------------------------------------
// 7. Lifecycle-disable a subtree root, verify cull bulk-skips via the fast
//    path (resolve has already propagated cascade-disable to children).
//    This exercises the O(1) bulk-skip case, not the O(skipCount) propagation.
// ---------------------------------------------------------------------------

void test_cull_lifecycle_disable_cascades_through_resolve(void)
{
    SCENE* scene = make_empty_scene_with_types(1, 0);

    SceneEntitySpec* root   = sceneSpecNewModel(0, (vec3){0.0f, 0.0f, 0.0f}, white());
    SceneEntitySpec* childA = sceneSpecNewModel(0, (vec3){1.0f, 0.0f, 0.0f}, white());
    SceneEntitySpec* childB = sceneSpecNewModel(0, (vec3){2.0f, 0.0f, 0.0f}, white());
    sceneSpecAddChild(root, childA);
    sceneSpecAddChild(root, childB);

    fillSceneFromSpecTree(scene, &root, 1);

    // Lifecycle-disable the root transform. Resolve will cascade to children.
    scene->transformManager.pTransformsHot[0].lifecycle = TRANSFORM_DISABLED;

    resolveTransforms(&scene->transformManager);

    // After resolve, all three transforms should have passFlags=DISABLED
    ASSERT_EQ(scene->transformManager.pTransformsHot[0].passFlags & TRANSFORM_PASS_DISABLED, TRANSFORM_PASS_DISABLED);
    ASSERT_EQ(scene->transformManager.pTransformsHot[1].passFlags & TRANSFORM_PASS_DISABLED, TRANSFORM_PASS_DISABLED);
    ASSERT_EQ(scene->transformManager.pTransformsHot[2].passFlags & TRANSFORM_PASS_DISABLED, TRANSFORM_PASS_DISABLED);

    // Run cull. Root is inside the frustum; without bulk-skip, cull would
    // test it and succeed. The test here is that the fast-path bulk-skip
    // fires: cull sees passFlags=DISABLED on root and skips the entire
    // subtree, leaving the DISABLED state intact.
    mat4 vp;
    make_test_view_proj(vp);
    FrustumPlanes frustum;
    frustumExtractPlanes(vp, &frustum);

    frustumCullScene(scene, &frustum);

    // Still all disabled — cull should not have cleared anything.
    ASSERT_EQ(scene->transformManager.pTransformsHot[0].passFlags & TRANSFORM_PASS_DISABLED, TRANSFORM_PASS_DISABLED);
    ASSERT_EQ(scene->transformManager.pTransformsHot[1].passFlags & TRANSFORM_PASS_DISABLED, TRANSFORM_PASS_DISABLED);
    ASSERT_EQ(scene->transformManager.pTransformsHot[2].passFlags & TRANSFORM_PASS_DISABLED, TRANSFORM_PASS_DISABLED);

    sceneSpecFree(root);
    free_empty_scene(scene);
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main(void)
{
    printf("=== Spec Tree Tests ===\n");

    RUN_TEST(test_flat_roots_three_models);
    RUN_TEST(test_single_root_two_children);
    RUN_TEST(test_cross_type_model_under_light);
    RUN_TEST(test_three_level_mixed_tree);
    RUN_TEST(test_balanced_tree_invariants);
    RUN_TEST(test_cull_frustum_fail_propagates_to_subtree);
    RUN_TEST(test_cull_lifecycle_disable_cascades_through_resolve);

    TEST_SUMMARY();
}
