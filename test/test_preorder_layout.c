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
// Preorder layout invariant tests.
//
// This file locks in the structural invariants that fillSceneFromSpecTree
// must maintain after the preorder refactor:
//
//   I1 — parent-before-child: for every transform i, parentIdx < i (or it's
//        TRANSFORM_NO_PARENT if i is a root).
//
//   I2 — subtree contiguity: for every transform i, its descendants occupy
//        exactly the range [i+1, i+1+skipCount[i]). Every index in that
//        range has i on its ancestor chain; no index outside the range has
//        i on its chain.
//
//   I3 — skipCount = subtree size: skipCount[i] equals the number of strict
//        descendants of i (0 for leaves).
//
//   I4 — root count invariant: for a spec tree with R roots, the sum
//        `Σ (1 + skipCount[root])` over all preorder-root positions equals
//        scene->transformManager.count.
//
// Exercised tree shapes:
//   - Single node (degenerate)
//   - Linear chain (depth-N degenerate spine)
//   - Wide fan (root with N siblings, no grandchildren)
//   - Rectangular grid of subtrees (multiple roots, each with fixed fan-out)
//   - Zigzag mixed (model→light→model→light chain)
//   - Asymmetric tree (root with children of different depths)
//
// The existing test_spec_tree.c already exercises the common cases
// (flat roots, two-child, three-level mixed, depth-4 balanced binary).
// This file focuses on edge shapes and explicit I1–I4 assertions.
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

// Walks `needle`'s ancestor chain and returns 1 iff `expected` is encountered.
static int is_ancestor_of(const TransformManager* tman, uint32_t expected, uint32_t needle)
{
    uint32_t cur = tman->pTransformsHot[needle].parentIdx;
    while (cur != TRANSFORM_NO_PARENT)
    {
        if (cur == expected) return 1;
        cur = tman->pTransformsHot[cur].parentIdx;
    }
    return 0;
}

// Full I1 / I2 / I3 / I4 check over a transform manager. `rootIndices` lists
// every preorder root position in insertion order; `rootCount` is its length.
static void assert_preorder_invariants(const TransformManager* tman,
                                       const uint32_t* rootIndices, uint32_t rootCount)
{
    // I1: parentIdx < i for non-roots.
    for (uint32_t i = 0; i < tman->count; i++)
    {
        uint32_t p = tman->pTransformsHot[i].parentIdx;
        if (p != TRANSFORM_NO_PARENT)
        {
            ASSERT_EQ(p < i, 1);
        }
    }

    // I2: every index in [i+1, i+1+skipCount) must be a descendant of i.
    //     every index NOT in that range must NOT be a descendant of i.
    for (uint32_t i = 0; i < tman->count; i++)
    {
        uint32_t skip = tman->pTransformsHot[i].skipCount;
        // In-range: all must be descendants of i.
        for (uint32_t j = i + 1; j < i + 1 + skip; j++)
        {
            ASSERT_EQ(j < tman->count, 1);
            ASSERT_EQ(is_ancestor_of(tman, i, j), 1);
        }
        // Out-of-range (past the subtree): none may have i as an ancestor.
        for (uint32_t j = i + 1 + skip; j < tman->count; j++)
        {
            ASSERT_EQ(is_ancestor_of(tman, i, j), 0);
        }
    }

    // I3: skipCount equals the number of strict descendants, computed by
    //     walking the parent chain from every other node.
    for (uint32_t i = 0; i < tman->count; i++)
    {
        uint32_t descendantCount = 0;
        for (uint32_t j = 0; j < tman->count; j++)
        {
            if (j != i && is_ancestor_of(tman, i, j)) descendantCount++;
        }
        ASSERT_EQ(tman->pTransformsHot[i].skipCount, descendantCount);
    }

    // I4: Σ (1 + skipCount[root]) over all roots = total count.
    uint32_t rootSum = 0;
    for (uint32_t r = 0; r < rootCount; r++)
    {
        uint32_t ri = rootIndices[r];
        ASSERT_EQ(tman->pTransformsHot[ri].parentIdx, TRANSFORM_NO_PARENT);
        rootSum += 1 + tman->pTransformsHot[ri].skipCount;
    }
    ASSERT_EQ(rootSum, tman->count);
}

// ---------------------------------------------------------------------------
// 1. Single node (degenerate): count=1, one root, skipCount=0.
// ---------------------------------------------------------------------------

void test_single_node_tree(void)
{
    SCENE* scene = make_empty_scene_with_types(1, 0);

    SceneEntitySpec* root = sceneSpecNewModel(0, (vec3){0.0f, 0.0f, 0.0f}, white());
    fillSceneFromSpecTree(scene, &root, 1);

    ASSERT_EQ(scene->transformManager.count, 1);
    ASSERT_EQ(scene->transformManager.pTransformsHot[0].parentIdx, TRANSFORM_NO_PARENT);
    ASSERT_EQ(scene->transformManager.pTransformsHot[0].skipCount, 0);

    uint32_t roots[] = {0};
    assert_preorder_invariants(&scene->transformManager, roots, 1);

    sceneSpecFree(root);
    free_empty_scene(scene);
}

// ---------------------------------------------------------------------------
// 2. Linear chain (depth-8 spine). Each node has exactly one child.
//    Preorder: [0, 1, 2, ..., 7]
//    skipCount[i] = 7 - i
// ---------------------------------------------------------------------------

void test_linear_chain_depth8(void)
{
    SCENE* scene = make_empty_scene_with_types(1, 0);

    const uint32_t depth = 8;
    SceneEntitySpec* nodes[8];
    for (uint32_t i = 0; i < depth; i++)
    {
        nodes[i] = sceneSpecNewModel(0, (vec3){(float) i, 0.0f, 0.0f}, white());
    }
    for (uint32_t i = 0; i + 1 < depth; i++)
    {
        sceneSpecAddChild(nodes[i], nodes[i + 1]);
    }

    fillSceneFromSpecTree(scene, &nodes[0], 1);

    ASSERT_EQ(scene->transformManager.count, depth);

    // I1 + structural chain: parent[i] = i-1 for i>0, root is 0.
    ASSERT_EQ(scene->transformManager.pTransformsHot[0].parentIdx, TRANSFORM_NO_PARENT);
    for (uint32_t i = 1; i < depth; i++)
    {
        ASSERT_EQ(scene->transformManager.pTransformsHot[i].parentIdx, i - 1);
    }
    // skipCount: leaf has 0, predecessor has 1, ..., root has depth-1.
    for (uint32_t i = 0; i < depth; i++)
    {
        ASSERT_EQ(scene->transformManager.pTransformsHot[i].skipCount, depth - 1 - i);
    }

    uint32_t roots[] = {0};
    assert_preorder_invariants(&scene->transformManager, roots, 1);

    sceneSpecFree(nodes[0]);
    free_empty_scene(scene);
}

// ---------------------------------------------------------------------------
// 3. Wide fan: root with 16 leaf children (depth 1, breadth 16).
//    Preorder: [root=0, leaf_0=1, leaf_1=2, ..., leaf_15=16]
//    root.skipCount = 16, every leaf's skipCount = 0.
// ---------------------------------------------------------------------------

void test_wide_fan_breadth16(void)
{
    SCENE* scene = make_empty_scene_with_types(1, 0);

    const uint32_t fan = 16;
    SceneEntitySpec* root = sceneSpecNewModel(0, (vec3){0.0f, 0.0f, 0.0f}, white());
    for (uint32_t c = 0; c < fan; c++)
    {
        SceneEntitySpec* child = sceneSpecNewModel(0, (vec3){(float)(c + 1), 0.0f, 0.0f}, white());
        sceneSpecAddChild(root, child);
    }

    fillSceneFromSpecTree(scene, &root, 1);

    ASSERT_EQ(scene->transformManager.count, fan + 1);
    ASSERT_EQ(scene->transformManager.pTransformsHot[0].parentIdx, TRANSFORM_NO_PARENT);
    ASSERT_EQ(scene->transformManager.pTransformsHot[0].skipCount, fan);

    for (uint32_t i = 1; i <= fan; i++)
    {
        ASSERT_EQ(scene->transformManager.pTransformsHot[i].parentIdx, 0);
        ASSERT_EQ(scene->transformManager.pTransformsHot[i].skipCount, 0);
    }

    uint32_t roots[] = {0};
    assert_preorder_invariants(&scene->transformManager, roots, 1);

    sceneSpecFree(root);
    free_empty_scene(scene);
}

// ---------------------------------------------------------------------------
// 4. Multiple independent subtrees (3 roots, each a depth-2 triangle).
//    Tree:
//        R0            R1            R2
//       /  \          /  \          /  \
//      A    B        C    D        E    F
//
//    Preorder: [R0=0, A=1, B=2, R1=3, C=4, D=5, R2=6, E=7, F=8]
//    I4: each root has skipCount=2; 3 * (1+2) = 9 = count.
// ---------------------------------------------------------------------------

void test_three_root_triangles(void)
{
    SCENE* scene = make_empty_scene_with_types(1, 0);

    SceneEntitySpec* r[3];
    for (uint32_t i = 0; i < 3; i++)
    {
        r[i] = sceneSpecNewModel(0, (vec3){(float) i, 0.0f, 0.0f}, white());
        SceneEntitySpec* a = sceneSpecNewModel(0, (vec3){(float) i, 1.0f, 0.0f}, white());
        SceneEntitySpec* b = sceneSpecNewModel(0, (vec3){(float) i, 2.0f, 0.0f}, white());
        sceneSpecAddChild(r[i], a);
        sceneSpecAddChild(r[i], b);
    }

    fillSceneFromSpecTree(scene, r, 3);

    ASSERT_EQ(scene->transformManager.count, 9);

    // Root positions should be 0, 3, 6 (each triangle occupies 3 slots in preorder).
    uint32_t expectedRoots[] = {0, 3, 6};
    for (uint32_t k = 0; k < 3; k++)
    {
        uint32_t idx = expectedRoots[k];
        ASSERT_EQ(scene->transformManager.pTransformsHot[idx].parentIdx, TRANSFORM_NO_PARENT);
        ASSERT_EQ(scene->transformManager.pTransformsHot[idx].skipCount, 2);
        ASSERT_EQ(scene->transformManager.pTransformsHot[idx + 1].parentIdx, idx);
        ASSERT_EQ(scene->transformManager.pTransformsHot[idx + 2].parentIdx, idx);
    }

    assert_preorder_invariants(&scene->transformManager, expectedRoots, 3);

    for (uint32_t i = 0; i < 3; i++) sceneSpecFree(r[i]);
    free_empty_scene(scene);
}

// ---------------------------------------------------------------------------
// 5. Zigzag mixed chain (model → light → model → light → model).
//    Only exercises the structural layout. Cross-type world-matrix
//    composition is separately verified in test_cross_type_bidirectional.c.
//
//    Preorder: [M0=0, L0=1, M1=2, L1=3, M2=4]
//    skipCount: [4, 3, 2, 1, 0]
// ---------------------------------------------------------------------------

void test_zigzag_mixed_chain(void)
{
    SCENE* scene = make_empty_scene_with_types(1, 1);

    SceneEntitySpec* m0 = sceneSpecNewModel(0, (vec3){0.0f, 0.0f, 0.0f}, white());
    SceneEntitySpec* l0 = sceneSpecNewLight(0, (vec3){1.0f, 0.0f, 0.0f}, 1.0f, white());
    SceneEntitySpec* m1 = sceneSpecNewModel(0, (vec3){2.0f, 0.0f, 0.0f}, white());
    SceneEntitySpec* l1 = sceneSpecNewLight(0, (vec3){3.0f, 0.0f, 0.0f}, 1.0f, white());
    SceneEntitySpec* m2 = sceneSpecNewModel(0, (vec3){4.0f, 0.0f, 0.0f}, white());
    sceneSpecAddChild(m0, l0);
    sceneSpecAddChild(l0, m1);
    sceneSpecAddChild(m1, l1);
    sceneSpecAddChild(l1, m2);

    fillSceneFromSpecTree(scene, &m0, 1);

    ASSERT_EQ(scene->transformManager.count, 5);
    ASSERT_EQ(scene->modelsManager.modelInstanceCapacity, 3);
    ASSERT_EQ(scene->lightManager.lightInstanceCapacity, 2);

    // Preorder chain: [M, L, M, L, M]
    const SceneEntityKind expectedKinds[5] = {
        SCENE_ENTITY_MODEL, SCENE_ENTITY_LIGHT, SCENE_ENTITY_MODEL, SCENE_ENTITY_LIGHT, SCENE_ENTITY_MODEL
    };
    for (uint32_t i = 0; i < 5; i++)
    {
        ASSERT_EQ(scene->transformManager.pPreorderEntities[i].kind, (uint16_t) expectedKinds[i]);
        ASSERT_EQ(scene->transformManager.pTransformsHot[i].skipCount, 4 - i);
    }
    ASSERT_EQ(scene->transformManager.pTransformsHot[0].parentIdx, TRANSFORM_NO_PARENT);
    for (uint32_t i = 1; i < 5; i++)
    {
        ASSERT_EQ(scene->transformManager.pTransformsHot[i].parentIdx, i - 1);
    }

    uint32_t roots[] = {0};
    assert_preorder_invariants(&scene->transformManager, roots, 1);

    sceneSpecFree(m0);
    free_empty_scene(scene);
}

// ---------------------------------------------------------------------------
// 6. Asymmetric tree — root with two branches of different depths.
//
//    Tree:
//              root
//              /  \
//             A    B
//            / \    \
//           C   D    E
//          /
//         F
//
//    Preorder (depth-first left-to-right): [root, A, C, F, D, B, E]
//    skipCount: [6, 3, 1, 0, 0, 1, 0]
// ---------------------------------------------------------------------------

void test_asymmetric_depth_tree(void)
{
    SCENE* scene = make_empty_scene_with_types(1, 0);

    SceneEntitySpec* root = sceneSpecNewModel(0, (vec3){0.0f, 0.0f, 0.0f}, white());
    SceneEntitySpec* a    = sceneSpecNewModel(0, (vec3){1.0f, 0.0f, 0.0f}, white());
    SceneEntitySpec* b    = sceneSpecNewModel(0, (vec3){2.0f, 0.0f, 0.0f}, white());
    SceneEntitySpec* c    = sceneSpecNewModel(0, (vec3){3.0f, 0.0f, 0.0f}, white());
    SceneEntitySpec* d    = sceneSpecNewModel(0, (vec3){4.0f, 0.0f, 0.0f}, white());
    SceneEntitySpec* e    = sceneSpecNewModel(0, (vec3){5.0f, 0.0f, 0.0f}, white());
    SceneEntitySpec* f    = sceneSpecNewModel(0, (vec3){6.0f, 0.0f, 0.0f}, white());

    sceneSpecAddChild(root, a);
    sceneSpecAddChild(root, b);
    sceneSpecAddChild(a, c);
    sceneSpecAddChild(a, d);
    sceneSpecAddChild(b, e);
    sceneSpecAddChild(c, f);

    fillSceneFromSpecTree(scene, &root, 1);

    ASSERT_EQ(scene->transformManager.count, 7);

    // Expected preorder parent chain: [NO, 0, 1, 2, 1, 0, 5]
    const uint32_t expectedParents[7] = {TRANSFORM_NO_PARENT, 0, 1, 2, 1, 0, 5};
    const uint32_t expectedSkipCounts[7] = {6, 3, 1, 0, 0, 1, 0};
    for (uint32_t i = 0; i < 7; i++)
    {
        ASSERT_EQ(scene->transformManager.pTransformsHot[i].parentIdx, expectedParents[i]);
        ASSERT_EQ(scene->transformManager.pTransformsHot[i].skipCount, expectedSkipCounts[i]);
    }

    uint32_t roots[] = {0};
    assert_preorder_invariants(&scene->transformManager, roots, 1);

    sceneSpecFree(root);
    free_empty_scene(scene);
}

// ---------------------------------------------------------------------------
// 7. Multi-root asymmetric: two roots with different shapes. Stress-tests
//    the I4 root-count invariant against uneven subtree sizes.
//
//    Tree:
//        R0 (chain of 3)              R1 (root with 4 leaves)
//         |                            /  |  |  \
//         X                           a   b   c   d
//         |
//         Y
//
//    Preorder: [R0=0, X=1, Y=2, R1=3, a=4, b=5, c=6, d=7]
//    skipCounts: [2, 1, 0, 4, 0, 0, 0, 0]
// ---------------------------------------------------------------------------

void test_multi_root_uneven(void)
{
    SCENE* scene = make_empty_scene_with_types(1, 0);

    // R0: chain of three
    SceneEntitySpec* r0 = sceneSpecNewModel(0, (vec3){0.0f, 0.0f, 0.0f}, white());
    SceneEntitySpec* x  = sceneSpecNewModel(0, (vec3){0.0f, 1.0f, 0.0f}, white());
    SceneEntitySpec* y  = sceneSpecNewModel(0, (vec3){0.0f, 2.0f, 0.0f}, white());
    sceneSpecAddChild(r0, x);
    sceneSpecAddChild(x,  y);

    // R1: root with four leaves
    SceneEntitySpec* r1 = sceneSpecNewModel(0, (vec3){5.0f, 0.0f, 0.0f}, white());
    SceneEntitySpec* a  = sceneSpecNewModel(0, (vec3){6.0f, 0.0f, 0.0f}, white());
    SceneEntitySpec* b  = sceneSpecNewModel(0, (vec3){7.0f, 0.0f, 0.0f}, white());
    SceneEntitySpec* c  = sceneSpecNewModel(0, (vec3){8.0f, 0.0f, 0.0f}, white());
    SceneEntitySpec* d  = sceneSpecNewModel(0, (vec3){9.0f, 0.0f, 0.0f}, white());
    sceneSpecAddChild(r1, a);
    sceneSpecAddChild(r1, b);
    sceneSpecAddChild(r1, c);
    sceneSpecAddChild(r1, d);

    SceneEntitySpec* roots[2] = {r0, r1};
    fillSceneFromSpecTree(scene, roots, 2);

    ASSERT_EQ(scene->transformManager.count, 8);

    // R0 subtree
    ASSERT_EQ(scene->transformManager.pTransformsHot[0].parentIdx, TRANSFORM_NO_PARENT);
    ASSERT_EQ(scene->transformManager.pTransformsHot[0].skipCount, 2);
    ASSERT_EQ(scene->transformManager.pTransformsHot[1].parentIdx, 0);
    ASSERT_EQ(scene->transformManager.pTransformsHot[1].skipCount, 1);
    ASSERT_EQ(scene->transformManager.pTransformsHot[2].parentIdx, 1);
    ASSERT_EQ(scene->transformManager.pTransformsHot[2].skipCount, 0);

    // R1 subtree
    ASSERT_EQ(scene->transformManager.pTransformsHot[3].parentIdx, TRANSFORM_NO_PARENT);
    ASSERT_EQ(scene->transformManager.pTransformsHot[3].skipCount, 4);
    for (uint32_t i = 4; i < 8; i++)
    {
        ASSERT_EQ(scene->transformManager.pTransformsHot[i].parentIdx, 3);
        ASSERT_EQ(scene->transformManager.pTransformsHot[i].skipCount, 0);
    }

    uint32_t rootIndices[] = {0, 3};
    assert_preorder_invariants(&scene->transformManager, rootIndices, 2);

    sceneSpecFree(r0);
    sceneSpecFree(r1);
    free_empty_scene(scene);
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main(void)
{
    printf("=== Preorder Layout Invariant Tests ===\n");

    RUN_TEST(test_single_node_tree);
    RUN_TEST(test_linear_chain_depth8);
    RUN_TEST(test_wide_fan_breadth16);
    RUN_TEST(test_three_root_triangles);
    RUN_TEST(test_zigzag_mixed_chain);
    RUN_TEST(test_asymmetric_depth_tree);
    RUN_TEST(test_multi_root_uneven);

    TEST_SUMMARY();
}
