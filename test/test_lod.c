#include "test_harness.h"
#include "test_helpers.h"
#include "uniformBuffer.h"
#include "transform.h"
#include "transformNode.h"
#include "model.h"
#include "cglm.h"
#include <string.h>

// ---------------------------------------------------------------------------
// LOD system tests
//
// Covers the runtime behavior of updateModelUniformBuffersDirect after the
// Phase 2 LOD partitioning was introduced:
//
//   L1 — per-instance screen-space LOD selection picks the correct bucket
//        based on distance from camera and bounding-sphere radius.
//   L2 — instances are written to the SSBO sorted by LOD within each type's
//        range (LOD 0 first, then LOD 1, then LOD 2).
//   L3 — sum(lodLiveInstanceCount[k]) == liveInstanceCount.
//   L4 — model->alive != 1 zeroes every LOD bucket for that model.
//   L5 — dead or cascade-disabled instances are excluded from all buckets.
//   L6 — multi-type scenes partition independently per type; instanceOffset[]
//        is monotonic and type bases don't overlap.
//
// Selection math (hardcoded 45° vertical FOV, matching updateFrameGlobals):
//   pixelScale = screenSize.y / 2 / tan(22.5°)
//   pixelSize  = modelRadius * pixelScale / distance(camera.eye, world.pos)
//
// With screenSize.y = 600: pixelScale = 300 / 0.41421 ≈ 724.26
// With modelRadius = 1.0 and Phase-1 thresholds (50/15/0):
//   dist <= 14.48  → LOD 0 (pixelSize >= 50)
//   dist <= 48.28  → LOD 1 (pixelSize >= 15)
//   dist  > 48.28  → LOD 2
//
// Test cases use distances well inside those bands to avoid boundary flakiness.
// ---------------------------------------------------------------------------

// Populate lod[0..2] + localAABBRadius with the Phase-1 defaults on every model.
// Offsets are synthetic — the tests don't read the vertex/index buffers, only
// the partitioning logic on top of them. Monotonic counts mirror a real asset.
static void install_synthetic_lod(Model3D* model, uint32_t typeIdx)
{
    model->localAABBRadius = 1.0f;

    // LOD 0: offset 0 + typeIdx*10000, 3000 indices
    model->lod[0].vertexOffset = typeIdx * 1000;
    model->lod[0].vertexCount  = 1000;
    model->lod[0].indexOffset  = typeIdx * 10000;
    model->lod[0].indexCount   = 3000;
    model->lod[0].minPixelSize = 50.0f;

    // LOD 1: ~30% of LOD 0
    model->lod[1].vertexOffset = typeIdx * 1000;
    model->lod[1].vertexCount  = 1000;
    model->lod[1].indexOffset  = typeIdx * 10000 + 3000;
    model->lod[1].indexCount   = 900;
    model->lod[1].minPixelSize = 15.0f;

    // LOD 2: ~10% of LOD 0
    model->lod[2].vertexOffset = typeIdx * 1000;
    model->lod[2].vertexCount  = 1000;
    model->lod[2].indexOffset  = typeIdx * 10000 + 3900;
    model->lod[2].indexCount   = 300;
    model->lod[2].minPixelSize = 0.0f;

    for (int k = 0; k < MODEL_LOD_COUNT; k++)
        model->lodLiveInstanceCount[k] = 0;
}

// Set an instance's world matrix translation column directly, bypassing the
// TRS → matrix resolve pipeline. The fill loop reads world->worldMatrix[3]
// (translation column) and nothing else from the matrix.
static void place_instance_at(SCENE* scene, uint32_t modelIdx, uint32_t instanceIdx, float x, float y, float z)
{
    Model3D*         model = &scene->modelsManager.pModel[modelIdx];
    Model3DInstance* inst  = &scene->modelsManager.pDenseModelInstances[model->denseBaseOffset + instanceIdx];
    TransformWorld*  world = &scene->transformManager.pTransformsWorld[inst->transformIdx];
    TransformHot*    hot   = &scene->transformManager.pTransformsHot[inst->transformIdx];

    glm_mat4_identity(world->worldMatrix);
    world->worldMatrix[3][0] = x;
    world->worldMatrix[3][1] = y;
    world->worldMatrix[3][2] = z;

    // Clear DIRTY so the UBO fill loop doesn't observe a half-resolved state.
    hot->passFlags &= ~TRANSFORM_PASS_DISABLED;
}

// Make a scene with configurable instances; camera at origin, screenSize set
// for the standard 600-tall viewport so the documented selection math holds.
static SCENE* make_lod_scene(uint32_t modelCount, uint32_t instancesPerModel)
{
    SCENE* scene = make_test_scene(modelCount, instancesPerModel);

    scene->camera.eye[0]       = 0.0f;
    scene->camera.eye[1]       = 0.0f;
    scene->camera.eye[2]       = 0.0f;
    scene->camera.screenSize[0] = 800.0f;
    scene->camera.screenSize[1] = 600.0f;

    for (uint32_t m = 0; m < modelCount; m++)
        install_synthetic_lod(&scene->modelsManager.pModel[m], m);

    return scene;
}

// ---------------------------------------------------------------------------
// L1 — Single-instance LOD classification
// ---------------------------------------------------------------------------

void test_lod_single_close_picks_lod0(void)
{
    SCENE* scene = make_lod_scene(1, 1);
    place_instance_at(scene, 0, 0, 5.0f, 0.0f, 0.0f);  // dist=5, pixelSize~145

    ModelInstanceUBO* ssbo = (ModelInstanceUBO*) calloc(4, sizeof(ModelInstanceUBO));
    updateModelUniformBuffersDirect(ssbo, scene);

    Model3D* m = &scene->modelsManager.pModel[0];
    ASSERT_EQ(m->lodLiveInstanceCount[0], 1);
    ASSERT_EQ(m->lodLiveInstanceCount[1], 0);
    ASSERT_EQ(m->lodLiveInstanceCount[2], 0);
    ASSERT_EQ(m->liveInstanceCount, 1);

    free(ssbo);
    free_test_scene(scene);
}

void test_lod_single_medium_picks_lod1(void)
{
    SCENE* scene = make_lod_scene(1, 1);
    place_instance_at(scene, 0, 0, 30.0f, 0.0f, 0.0f);  // dist=30, pixelSize~24

    ModelInstanceUBO* ssbo = (ModelInstanceUBO*) calloc(4, sizeof(ModelInstanceUBO));
    updateModelUniformBuffersDirect(ssbo, scene);

    Model3D* m = &scene->modelsManager.pModel[0];
    ASSERT_EQ(m->lodLiveInstanceCount[0], 0);
    ASSERT_EQ(m->lodLiveInstanceCount[1], 1);
    ASSERT_EQ(m->lodLiveInstanceCount[2], 0);
    ASSERT_EQ(m->liveInstanceCount, 1);

    free(ssbo);
    free_test_scene(scene);
}

void test_lod_single_far_picks_lod2(void)
{
    SCENE* scene = make_lod_scene(1, 1);
    place_instance_at(scene, 0, 0, 100.0f, 0.0f, 0.0f);  // dist=100, pixelSize~7.2

    ModelInstanceUBO* ssbo = (ModelInstanceUBO*) calloc(4, sizeof(ModelInstanceUBO));
    updateModelUniformBuffersDirect(ssbo, scene);

    Model3D* m = &scene->modelsManager.pModel[0];
    ASSERT_EQ(m->lodLiveInstanceCount[0], 0);
    ASSERT_EQ(m->lodLiveInstanceCount[1], 0);
    ASSERT_EQ(m->lodLiveInstanceCount[2], 1);
    ASSERT_EQ(m->liveInstanceCount, 1);

    free(ssbo);
    free_test_scene(scene);
}

// ---------------------------------------------------------------------------
// L2/L3 — Mixed distances + partitioned SSBO ordering
// ---------------------------------------------------------------------------

void test_lod_mixed_distances_populate_all_buckets(void)
{
    SCENE* scene = make_lod_scene(1, 6);

    // 2× close (LOD 0), 3× medium (LOD 1), 1× far (LOD 2)
    place_instance_at(scene, 0, 0,   5.0f, 0.0f, 0.0f);
    place_instance_at(scene, 0, 1,   8.0f, 0.0f, 0.0f);
    place_instance_at(scene, 0, 2,  25.0f, 0.0f, 0.0f);
    place_instance_at(scene, 0, 3,  30.0f, 0.0f, 0.0f);
    place_instance_at(scene, 0, 4,  40.0f, 0.0f, 0.0f);
    place_instance_at(scene, 0, 5, 100.0f, 0.0f, 0.0f);

    ModelInstanceUBO* ssbo = (ModelInstanceUBO*) calloc(8, sizeof(ModelInstanceUBO));
    updateModelUniformBuffersDirect(ssbo, scene);

    Model3D* m = &scene->modelsManager.pModel[0];
    ASSERT_EQ(m->lodLiveInstanceCount[0], 2);
    ASSERT_EQ(m->lodLiveInstanceCount[1], 3);
    ASSERT_EQ(m->lodLiveInstanceCount[2], 1);
    ASSERT_EQ(m->liveInstanceCount, 6);

    free(ssbo);
    free_test_scene(scene);
}

void test_lod_partitioning_order_in_ssbo(void)
{
    // Place instances so the SSBO partitions clearly: LOD 0 slots hold x in
    // {5,8}, LOD 1 slots hold x in {25,30,40}, LOD 2 slot holds x=100.
    // Within each bucket, order mirrors traversal order of alive instances
    // (not spatial sort), so the test asserts set membership per bucket.
    SCENE* scene = make_lod_scene(1, 6);
    place_instance_at(scene, 0, 0,  30.0f, 0.0f, 0.0f);  // LOD 1
    place_instance_at(scene, 0, 1,   5.0f, 0.0f, 0.0f);  // LOD 0
    place_instance_at(scene, 0, 2, 100.0f, 0.0f, 0.0f);  // LOD 2
    place_instance_at(scene, 0, 3,  25.0f, 0.0f, 0.0f);  // LOD 1
    place_instance_at(scene, 0, 4,   8.0f, 0.0f, 0.0f);  // LOD 0
    place_instance_at(scene, 0, 5,  40.0f, 0.0f, 0.0f);  // LOD 1

    ModelInstanceUBO* ssbo = (ModelInstanceUBO*) calloc(8, sizeof(ModelInstanceUBO));
    updateModelUniformBuffersDirect(ssbo, scene);

    Model3D* m = &scene->modelsManager.pModel[0];
    ASSERT_EQ(m->lodLiveInstanceCount[0], 2);
    ASSERT_EQ(m->lodLiveInstanceCount[1], 3);
    ASSERT_EQ(m->lodLiveInstanceCount[2], 1);

    // Slots [0..1] must be LOD-0 x-values (5 or 8)
    for (uint32_t s = 0; s < 2; s++)
    {
        float x = ssbo[s].model[3][0];
        if (x != 5.0f && x != 8.0f)
        {
            printf("FAIL\n    slot %u LOD-0 region: expected x in {5,8}, got %.2f\n", s, x);
            tests_failed++;
            free(ssbo);
            free_test_scene(scene);
            return;
        }
    }
    // Slots [2..4] must be LOD-1 x-values (25, 30, or 40)
    for (uint32_t s = 2; s < 5; s++)
    {
        float x = ssbo[s].model[3][0];
        if (x != 25.0f && x != 30.0f && x != 40.0f)
        {
            printf("FAIL\n    slot %u LOD-1 region: expected x in {25,30,40}, got %.2f\n", s, x);
            tests_failed++;
            free(ssbo);
            free_test_scene(scene);
            return;
        }
    }
    // Slot [5] must be LOD-2 (x=100)
    ASSERT_FLOAT_EQ(ssbo[5].model[3][0], 100.0f);

    free(ssbo);
    free_test_scene(scene);
}

// ---------------------------------------------------------------------------
// L4 — Disabled model
// ---------------------------------------------------------------------------

void test_lod_disabled_model_zero_buckets(void)
{
    SCENE* scene = make_lod_scene(1, 4);
    for (uint32_t i = 0; i < 4; i++)
        place_instance_at(scene, 0, i, 5.0f, 0.0f, 0.0f);  // would be LOD 0 if alive

    scene->modelsManager.pModel[0].alive = 2;  // MODEL_DISABLED

    ModelInstanceUBO* ssbo = (ModelInstanceUBO*) calloc(8, sizeof(ModelInstanceUBO));
    updateModelUniformBuffersDirect(ssbo, scene);

    Model3D* m = &scene->modelsManager.pModel[0];
    ASSERT_EQ(m->lodLiveInstanceCount[0], 0);
    ASSERT_EQ(m->lodLiveInstanceCount[1], 0);
    ASSERT_EQ(m->lodLiveInstanceCount[2], 0);
    ASSERT_EQ(m->liveInstanceCount, 0);
    ASSERT_EQ(scene->modelsManager.modelInstanceLen, 0);

    free(ssbo);
    free_test_scene(scene);
}

// ---------------------------------------------------------------------------
// L5 — Dead / cascade-disabled instances excluded
// ---------------------------------------------------------------------------

void test_lod_dead_instances_skipped(void)
{
    SCENE* scene = make_lod_scene(1, 4);
    place_instance_at(scene, 0, 0,   5.0f, 0.0f, 0.0f);  // LOD 0
    place_instance_at(scene, 0, 1,  30.0f, 0.0f, 0.0f);  // LOD 1 — will be killed
    place_instance_at(scene, 0, 2,  35.0f, 0.0f, 0.0f);  // LOD 1
    place_instance_at(scene, 0, 3, 100.0f, 0.0f, 0.0f);  // LOD 2

    Model3D* m0 = &scene->modelsManager.pModel[0];
    scene->modelsManager.pDenseModelInstances[m0->denseBaseOffset + 1].lifecycle = INSTANCE_DESTROYED;

    ModelInstanceUBO* ssbo = (ModelInstanceUBO*) calloc(8, sizeof(ModelInstanceUBO));
    updateModelUniformBuffersDirect(ssbo, scene);

    ASSERT_EQ(m0->lodLiveInstanceCount[0], 1);
    ASSERT_EQ(m0->lodLiveInstanceCount[1], 1);  // one survivor in LOD 1
    ASSERT_EQ(m0->lodLiveInstanceCount[2], 1);
    ASSERT_EQ(m0->liveInstanceCount, 3);

    free(ssbo);
    free_test_scene(scene);
}

void test_lod_cascade_disabled_skipped(void)
{
    SCENE* scene = make_lod_scene(1, 3);
    place_instance_at(scene, 0, 0,  5.0f, 0.0f, 0.0f);
    place_instance_at(scene, 0, 1, 30.0f, 0.0f, 0.0f);
    place_instance_at(scene, 0, 2, 30.0f, 0.0f, 0.0f);

    // Mark the second instance as cascade-disabled by hierarchy resolve
    Model3D*         m0   = &scene->modelsManager.pModel[0];
    Model3DInstance* inst = &scene->modelsManager.pDenseModelInstances[m0->denseBaseOffset + 1];
    scene->transformManager.pTransformsHot[inst->transformIdx].passFlags |= TRANSFORM_PASS_DISABLED;

    ModelInstanceUBO* ssbo = (ModelInstanceUBO*) calloc(4, sizeof(ModelInstanceUBO));
    updateModelUniformBuffersDirect(ssbo, scene);

    ASSERT_EQ(m0->lodLiveInstanceCount[0], 1);
    ASSERT_EQ(m0->lodLiveInstanceCount[1], 1);
    ASSERT_EQ(m0->lodLiveInstanceCount[2], 0);
    ASSERT_EQ(m0->liveInstanceCount, 2);

    free(ssbo);
    free_test_scene(scene);
}

// ---------------------------------------------------------------------------
// L3 — sum(lodLiveInstanceCount) == liveInstanceCount invariant (exhaustive)
// ---------------------------------------------------------------------------

void test_lod_sum_equals_live_count(void)
{
    SCENE* scene = make_lod_scene(1, 5);
    place_instance_at(scene, 0, 0,   5.0f, 0.0f, 0.0f);
    place_instance_at(scene, 0, 1,  30.0f, 0.0f, 0.0f);
    place_instance_at(scene, 0, 2, 100.0f, 0.0f, 0.0f);
    place_instance_at(scene, 0, 3,   8.0f, 0.0f, 0.0f);
    place_instance_at(scene, 0, 4, 200.0f, 0.0f, 0.0f);

    ModelInstanceUBO* ssbo = (ModelInstanceUBO*) calloc(8, sizeof(ModelInstanceUBO));
    updateModelUniformBuffersDirect(ssbo, scene);

    Model3D* m   = &scene->modelsManager.pModel[0];
    uint32_t sum = m->lodLiveInstanceCount[0] + m->lodLiveInstanceCount[1] + m->lodLiveInstanceCount[2];
    ASSERT_EQ(sum, m->liveInstanceCount);
    ASSERT_EQ(sum, 5);

    free(ssbo);
    free_test_scene(scene);
}

// ---------------------------------------------------------------------------
// L6 — Multi-type independent partitioning + instanceOffset monotonicity
// ---------------------------------------------------------------------------

void test_lod_multi_type_independent_partitioning(void)
{
    SCENE* scene = make_lod_scene(2, 4);

    // Type 0: 2 close, 2 medium
    place_instance_at(scene, 0, 0,  5.0f, 0.0f, 0.0f);
    place_instance_at(scene, 0, 1,  8.0f, 0.0f, 0.0f);
    place_instance_at(scene, 0, 2, 30.0f, 0.0f, 0.0f);
    place_instance_at(scene, 0, 3, 40.0f, 0.0f, 0.0f);

    // Type 1: 1 close, 1 medium, 2 far
    place_instance_at(scene, 1, 0,   6.0f, 0.0f, 0.0f);
    place_instance_at(scene, 1, 1,  35.0f, 0.0f, 0.0f);
    place_instance_at(scene, 1, 2, 100.0f, 0.0f, 0.0f);
    place_instance_at(scene, 1, 3, 150.0f, 0.0f, 0.0f);

    ModelInstanceUBO* ssbo = (ModelInstanceUBO*) calloc(16, sizeof(ModelInstanceUBO));
    updateModelUniformBuffersDirect(ssbo, scene);

    Model3D* m0 = &scene->modelsManager.pModel[0];
    Model3D* m1 = &scene->modelsManager.pModel[1];

    ASSERT_EQ(m0->lodLiveInstanceCount[0], 2);
    ASSERT_EQ(m0->lodLiveInstanceCount[1], 2);
    ASSERT_EQ(m0->lodLiveInstanceCount[2], 0);
    ASSERT_EQ(m0->liveInstanceCount, 4);

    ASSERT_EQ(m1->lodLiveInstanceCount[0], 1);
    ASSERT_EQ(m1->lodLiveInstanceCount[1], 1);
    ASSERT_EQ(m1->lodLiveInstanceCount[2], 2);
    ASSERT_EQ(m1->liveInstanceCount, 4);

    // Type bases: type 0 starts at 0, type 1 starts at type 0's live count (4)
    ASSERT_EQ(scene->modelsManager.instanceOffset.items[0], 0);
    ASSERT_EQ(scene->modelsManager.instanceOffset.items[1], 4);
    ASSERT_EQ(scene->modelsManager.modelInstanceLen, 8);

    free(ssbo);
    free_test_scene(scene);
}

// ---------------------------------------------------------------------------
// Structural invariants on synthetic LOD data
// ---------------------------------------------------------------------------

void test_lod_indices_do_not_overlap_between_levels(void)
{
    // After install_synthetic_lod, LOD 0 occupies [0, 3000), LOD 1 [3000, 3900),
    // LOD 2 [3900, 4200). Assert no overlap and monotonically non-increasing
    // counts — the same pattern meshopt_simplify produces at scene load.
    SCENE* scene = make_lod_scene(1, 1);
    Model3D* m = &scene->modelsManager.pModel[0];

    // Non-increasing index counts (full > medium > coarse)
    if (!(m->lod[0].indexCount >= m->lod[1].indexCount &&
          m->lod[1].indexCount >= m->lod[2].indexCount))
    {
        printf("FAIL\n    expected lod[0].idxCount >= lod[1] >= lod[2], got %u, %u, %u\n",
               m->lod[0].indexCount, m->lod[1].indexCount, m->lod[2].indexCount);
        tests_failed++;
        free_test_scene(scene);
        return;
    }

    // Non-increasing minPixelSize (finest LOD has highest threshold)
    if (!(m->lod[0].minPixelSize >= m->lod[1].minPixelSize &&
          m->lod[1].minPixelSize >= m->lod[2].minPixelSize))
    {
        printf("FAIL\n    expected lod[0].minPixelSize >= lod[1] >= lod[2], got %.2f, %.2f, %.2f\n",
               m->lod[0].minPixelSize, m->lod[1].minPixelSize, m->lod[2].minPixelSize);
        tests_failed++;
        free_test_scene(scene);
        return;
    }

    // No index-range overlap
    for (int a = 0; a < MODEL_LOD_COUNT; a++)
    {
        uint32_t aStart = m->lod[a].indexOffset;
        uint32_t aEnd   = aStart + m->lod[a].indexCount;
        for (int b = a + 1; b < MODEL_LOD_COUNT; b++)
        {
            uint32_t bStart = m->lod[b].indexOffset;
            uint32_t bEnd   = bStart + m->lod[b].indexCount;
            if (!(aEnd <= bStart || bEnd <= aStart))
            {
                printf("FAIL\n    LOD %d [%u,%u) overlaps LOD %d [%u,%u)\n",
                       a, aStart, aEnd, b, bStart, bEnd);
                tests_failed++;
                free_test_scene(scene);
                return;
            }
        }
    }

    // lod[2].minPixelSize must be 0 so the selection is total (every instance
    // at least hits the coarsest bucket — no instances fall off the end).
    ASSERT_FLOAT_EQ(m->lod[MODEL_LOD_COUNT - 1].minPixelSize, 0.0f);

    free_test_scene(scene);
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------

int main(void)
{
    printf("\n=== LOD system tests ===\n");

    RUN_TEST(test_lod_single_close_picks_lod0);
    RUN_TEST(test_lod_single_medium_picks_lod1);
    RUN_TEST(test_lod_single_far_picks_lod2);
    RUN_TEST(test_lod_mixed_distances_populate_all_buckets);
    RUN_TEST(test_lod_partitioning_order_in_ssbo);
    RUN_TEST(test_lod_disabled_model_zero_buckets);
    RUN_TEST(test_lod_dead_instances_skipped);
    RUN_TEST(test_lod_cascade_disabled_skipped);
    RUN_TEST(test_lod_sum_equals_live_count);
    RUN_TEST(test_lod_multi_type_independent_partitioning);
    RUN_TEST(test_lod_indices_do_not_overlap_between_levels);

    TEST_SUMMARY();
}
