#include "test_harness.h"
#include "test_helpers.h"
#include "uniformBuffer.h"
#include "transform.h"
#include "transformNode.h"
#include "scene.h"

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static GraphicSwapchain make_fake_swapchain(void)
{
    GraphicSwapchain sc;
    memset(&sc, 0, sizeof(sc));
    sc.currentExtent2D.width  = 800;
    sc.currentExtent2D.height = 600;
    return sc;
}

static Camera3D make_test_camera(void)
{
    Camera3D cam;
    memset(&cam, 0, sizeof(cam));
    cam.eye[2] = 5.0f;
    cam.up[1]  = 1.0f;
    return cam;
}

// ---------------------------------------------------------------------------
// 1. Varied instance counts — denseBaseOffset correctness
// ---------------------------------------------------------------------------

void test_varied_instance_counts_offsets(void)
{
    // Model 0: 2 instances, Model 1: 5, Model 2: 1
    uint32_t counts[] = {2, 5, 1};
    Models3DManager man = make_test_manager_varied(3, counts);

    ASSERT_EQ(man.pModel[0].denseBaseOffset, 0);
    ASSERT_EQ(man.pModel[1].denseBaseOffset, 2);
    ASSERT_EQ(man.pModel[2].denseBaseOffset, 7);
    ASSERT_EQ(man.modelInstanceCapacity, 8);

    // Every instance should be alive
    for (uint32_t i = 0; i < 8; i++)
        ASSERT_EQ(man.pDenseModelInstances[i].lifecycle, INSTANCE_ALIVE);

    free_test_manager(&man);
}

void test_varied_counts_lifecycle_operations(void)
{
    uint32_t counts[] = {2, 5, 1};
    Models3DManager man = make_test_manager_varied(3, counts);

    // Destroy instance 4 of model 1 (the last one) — global index 2+4=6
    destroyInstance(&man, 1, 4);
    ASSERT_EQ(man.pDenseModelInstances[6].lifecycle, INSTANCE_DESTROYED);

    // Disable instance 0 of model 2 — global index 7
    disableInstance(&man, 2, 0);
    ASSERT_EQ(man.pDenseModelInstances[7].lifecycle, INSTANCE_DISABLED);

    // Model 0 instances untouched
    ASSERT_EQ(man.pDenseModelInstances[0].lifecycle, INSTANCE_ALIVE);
    ASSERT_EQ(man.pDenseModelInstances[1].lifecycle, INSTANCE_ALIVE);

    free_test_manager(&man);
}

// ---------------------------------------------------------------------------
// 2. Dense array boundary isolation
// ---------------------------------------------------------------------------

void test_dense_boundary_no_cross_contamination(void)
{
    uint32_t counts[] = {3, 3};
    Models3DManager  man  = make_test_manager_varied(2, counts);
    TransformManager tman = make_test_transform_manager(6);

    // Assign transformIdx
    for (uint32_t i = 0; i < 6; i++)
        man.pDenseModelInstances[i].transformIdx = i;

    // Set distinct positions on boundary instances
    transformSetPosition(&tman.pTransformsCold[2], (vec3){99.0f, 0.0f, 0.0f});
    transformSetPosition(&tman.pTransformsCold[3], (vec3){77.0f, 0.0f, 0.0f});

    // Destroy model 0's last instance
    destroyInstance(&man, 0, 2);

    // Model 1's first instance must be untouched
    ASSERT_EQ(man.pDenseModelInstances[3].lifecycle, INSTANCE_ALIVE);
    ASSERT_FLOAT_EQ(tman.pTransformsCold[3].position[0], 77.0f);

    // Destroy model 1's first instance
    destroyInstance(&man, 1, 0);

    // Model 0's second instance must be untouched
    ASSERT_EQ(man.pDenseModelInstances[1].lifecycle, INSTANCE_ALIVE);

    free_test_manager(&man);
    free_test_transform_manager(&tman);
}

// ---------------------------------------------------------------------------
// 3. resolveGlobalInstance
// ---------------------------------------------------------------------------

void test_resolve_global_instance(void)
{
    uint32_t counts[] = {2, 3};
    Models3DManager  man  = make_test_manager_varied(2, counts);
    TransformManager tman = make_test_transform_manager(5);

    for (uint32_t i = 0; i < 5; i++)
        man.pDenseModelInstances[i].transformIdx = i;

    // Set a position on model 1, instance 1 (global index = 2+1 = 3)
    transformSetPosition(&tman.pTransformsCold[3], (vec3){42.0f, 0.0f, 0.0f});

    uint32_t globalIdx = computeGlobalInstanceIndex(&man.pModel[1], 1);
    ASSERT_EQ(globalIdx, 3);

    Model3DInstance* inst = resolveGlobalInstance(&man, globalIdx);
    ASSERT_NOT_NULL(inst);
    ASSERT_FLOAT_EQ(tman.pTransformsCold[inst->transformIdx].position[0], 42.0f);

    free_test_manager(&man);
    free_test_transform_manager(&tman);
}

void test_resolve_global_instance_first_and_last(void)
{
    uint32_t counts[] = {4, 2, 3};
    Models3DManager man = make_test_manager_varied(3, counts);

    // First instance in entire array
    Model3DInstance* first = resolveGlobalInstance(&man, 0);
    ASSERT_NOT_NULL(first);
    ASSERT_EQ(first->lifecycle, INSTANCE_ALIVE);

    // Last instance (global index 4+2+3-1 = 8)
    Model3DInstance* last = resolveGlobalInstance(&man, 8);
    ASSERT_NOT_NULL(last);
    ASSERT_EQ(last->lifecycle, INSTANCE_ALIVE);

    free_test_manager(&man);
}

// ---------------------------------------------------------------------------
// 4. queryInstanceAlive on disabled (not destroyed) model
// ---------------------------------------------------------------------------

void test_query_instance_alive_on_disabled_model(void)
{
    Models3DManager man = make_test_manager(1, 4);

    disableModel(&man, 0);

    ASSERT_EQ(man.pModel[0].alive, MODEL_DISABLED);

    // Instances are still alive in the dense array
    int state = queryInstanceAlive(&man, 0, 0);
    ASSERT_EQ(state, INSTANCE_ALIVE);

    // Destroy an instance while model is disabled, then query
    destroyInstance(&man, 0, 1);
    ASSERT_EQ(queryInstanceAlive(&man, 0, 1), INSTANCE_DESTROYED);

    free_test_manager(&man);
}

void test_query_instance_alive_on_destroyed_model(void)
{
    Models3DManager man = make_test_manager(1, 4);

    destroyModel(&man, 0);

    // Model is destroyed (alive=0). !0 == 1 (true) -> returns -1.
    ASSERT_EQ(queryInstanceAlive(&man, 0, 0), -1);

    free_test_manager(&man);
}

// ---------------------------------------------------------------------------
// 5. UBO fill multi-frame simulation
// ---------------------------------------------------------------------------

void test_ubo_fill_multi_frame_livecount_stability(void)
{
    SCENE*           scene = make_test_scene(2, 3);
    GraphicSwapchain sc    = make_fake_swapchain();
    Camera3D         cam   = make_test_camera();

    // Frame 1: all alive
    resolveTransforms(&scene->transformManager);
    ModelInstanceUBO* r1 = (ModelInstanceUBO*) malloc(sizeof(ModelInstanceUBO) * scene->modelsManager.modelInstanceCapacity);
    updateModelUniformBuffersDirect(r1, scene);
    ASSERT_EQ(scene->modelsManager.pModel[0].liveInstanceCount, 3);
    ASSERT_EQ(scene->modelsManager.pModel[1].liveInstanceCount, 3);
    ASSERT_EQ(scene->modelsManager.modelInstanceLen, 6);
    free(r1);

    // Kill one instance between frames
    destroyInstance(&scene->modelsManager, 0, 1);

    // Frame 2: model 0 should have 2 live
    resolveTransforms(&scene->transformManager);
    ModelInstanceUBO* r2 = (ModelInstanceUBO*) malloc(sizeof(ModelInstanceUBO) * scene->modelsManager.modelInstanceCapacity);
    updateModelUniformBuffersDirect(r2, scene);
    ASSERT_EQ(scene->modelsManager.pModel[0].liveInstanceCount, 2);
    ASSERT_EQ(scene->modelsManager.pModel[1].liveInstanceCount, 3);
    ASSERT_EQ(scene->modelsManager.modelInstanceLen, 5);
    free(r2);

    // Re-enable won't work (destroyed), disable model 1 instead
    disableModel(&scene->modelsManager, 1);

    // Frame 3: model 1 entirely skipped
    resolveTransforms(&scene->transformManager);
    ModelInstanceUBO* r3 = (ModelInstanceUBO*) malloc(sizeof(ModelInstanceUBO) * scene->modelsManager.modelInstanceCapacity);
    updateModelUniformBuffersDirect(r3, scene);
    ASSERT_EQ(scene->modelsManager.pModel[0].liveInstanceCount, 2);
    ASSERT_EQ(scene->modelsManager.pModel[1].liveInstanceCount, 0);
    ASSERT_EQ(scene->modelsManager.modelInstanceLen, 2);
    free(r3);

    // Re-enable model 1
    enableModel(&scene->modelsManager, 1);

    // Frame 4: model 1 back to 3
    resolveTransforms(&scene->transformManager);
    ModelInstanceUBO* r4 = (ModelInstanceUBO*) malloc(sizeof(ModelInstanceUBO) * scene->modelsManager.modelInstanceCapacity);
    updateModelUniformBuffersDirect(r4, scene);
    ASSERT_EQ(scene->modelsManager.pModel[0].liveInstanceCount, 2);
    ASSERT_EQ(scene->modelsManager.pModel[1].liveInstanceCount, 3);
    ASSERT_EQ(scene->modelsManager.modelInstanceLen, 5);
    free(r4);

    free_test_scene(scene);
}

// ---------------------------------------------------------------------------
// 6. Destroy model then UBO fill — dense slots skipped
// ---------------------------------------------------------------------------

void test_destroy_model_then_ubo_fill(void)
{
    SCENE*           scene = make_test_scene(2, 3);
    GraphicSwapchain sc    = make_fake_swapchain();
    Camera3D         cam   = make_test_camera();

    destroyModel(&scene->modelsManager, 0);

    resolveTransforms(&scene->transformManager);
    ModelInstanceUBO* result = (ModelInstanceUBO*) malloc(sizeof(ModelInstanceUBO) * scene->modelsManager.modelInstanceCapacity);
    updateModelUniformBuffersDirect(result, scene);

    // Model 0 dead -> 0 live, Model 1 alive -> 3 live
    ASSERT_EQ(scene->modelsManager.pModel[0].liveInstanceCount, 0);
    ASSERT_EQ(scene->modelsManager.pModel[1].liveInstanceCount, 3);
    ASSERT_EQ(scene->modelsManager.modelInstanceLen, 3);

    // Instance offset for model 1 should be 0 (compacted past dead model 0)
    ASSERT_EQ(scene->modelsManager.instanceOffset.items[1], 0);

    free(result);
    free_test_scene(scene);
}

// ---------------------------------------------------------------------------
// 7. Varied counts + UBO compaction
// ---------------------------------------------------------------------------

static SCENE* make_test_scene_varied(uint32_t modelCount, const uint32_t* instanceCounts)
{
    SCENE* scene = (SCENE*) calloc(1, sizeof(SCENE));

    uint32_t total = 0;
    for (uint32_t m = 0; m < modelCount; m++)
        total += instanceCounts[m];

    scene->transformManager = make_test_transform_manager(total);
    scene->modelsManager    = make_test_manager_varied(modelCount, instanceCounts);

    // Wire transformIdx
    for (uint32_t i = 0; i < total; i++)
        scene->modelsManager.pDenseModelInstances[i].transformIdx = i;

    scene->camera.eye[2] = 5.0f;
    scene->camera.up[1]  = 1.0f;

    return scene;
}

void test_varied_counts_ubo_compaction(void)
{
    uint32_t counts[] = {2, 5, 1};
    SCENE*           scene = make_test_scene_varied(3, counts);
    GraphicSwapchain sc    = make_fake_swapchain();
    Camera3D         cam   = make_test_camera();

    // Kill 1 instance from model 1 (the big one)
    destroyInstance(&scene->modelsManager, 1, 2);

    resolveTransforms(&scene->transformManager);
    ModelInstanceUBO* result = (ModelInstanceUBO*) malloc(sizeof(ModelInstanceUBO) * scene->modelsManager.modelInstanceCapacity);
    updateModelUniformBuffersDirect(result, scene);

    ASSERT_EQ(scene->modelsManager.pModel[0].liveInstanceCount, 2);
    ASSERT_EQ(scene->modelsManager.pModel[1].liveInstanceCount, 4);
    ASSERT_EQ(scene->modelsManager.pModel[2].liveInstanceCount, 1);
    ASSERT_EQ(scene->modelsManager.modelInstanceLen, 7);

    // Offsets should be compacted: 0, 2, 6
    ASSERT_EQ(scene->modelsManager.instanceOffset.items[0], 0);
    ASSERT_EQ(scene->modelsManager.instanceOffset.items[1], 2);
    ASSERT_EQ(scene->modelsManager.instanceOffset.items[2], 6);

    free(result);
    free_test_scene(scene);
}

// ---------------------------------------------------------------------------
// 8. Staging consolidation via fillModelInstancesIndices
// ---------------------------------------------------------------------------

void test_staging_consolidation(void)
{
    // Simulate the real init path: allocate staging, call fillModelInstancesIndices
    Models3DManager man;
    memset(&man, 0, sizeof(man));

    TransformManager tman;
    memset(&tman, 0, sizeof(tman));

    man.modelLen = 2;
    man.pModel   = (Model3D*) calloc(2, sizeof(Model3D));

    // Model 0: 3 instances with staging transforms
    man.pModel[0].instanceCount      = 3;
    man.pModel[0].alive              = MODEL_ALIVE;
    man.pModel[0].liveInstanceCount  = 0;
    man.pModel[0].pStagingInstances  = (Model3DInstance*) calloc(3, sizeof(Model3DInstance));
    man.pModel[0].pStagingTransforms = (TransformCold*) calloc(3, sizeof(TransformCold));
    for (uint32_t i = 0; i < 3; i++)
    {
        man.pModel[0].pStagingInstances[i].lifecycle = INSTANCE_ALIVE;
        transformInitCold(&man.pModel[0].pStagingTransforms[i]);
        transformSetPosition(&man.pModel[0].pStagingTransforms[i], (vec3){(float)i * 10.0f, 0.0f, 0.0f});
    }

    // Model 1: 2 instances
    man.pModel[1].instanceCount      = 2;
    man.pModel[1].alive              = MODEL_ALIVE;
    man.pModel[1].liveInstanceCount  = 0;
    man.pModel[1].pStagingInstances  = (Model3DInstance*) calloc(2, sizeof(Model3DInstance));
    man.pModel[1].pStagingTransforms = (TransformCold*) calloc(2, sizeof(TransformCold));
    for (uint32_t i = 0; i < 2; i++)
    {
        man.pModel[1].pStagingInstances[i].lifecycle = INSTANCE_ALIVE;
        transformInitCold(&man.pModel[1].pStagingTransforms[i]);
        transformSetPosition(&man.pModel[1].pStagingTransforms[i], (vec3){100.0f + (float)i, 0.0f, 0.0f});
    }

    man.modelInstanceCapacity = 5;

    // This is the function under test
    fillModelInstancesIndices(&man, &tman);

    // Verify offsets
    ASSERT_EQ(man.pModel[0].denseBaseOffset, 0);
    ASSERT_EQ(man.pModel[1].denseBaseOffset, 3);

    // Verify dense array has the data from staging
    ASSERT_NOT_NULL(man.pDenseModelInstances);

    // Transform positions should be in the transform manager
    ASSERT_FLOAT_EQ(tman.pTransformsCold[man.pDenseModelInstances[0].transformIdx].position[0], 0.0f);
    ASSERT_FLOAT_EQ(tman.pTransformsCold[man.pDenseModelInstances[1].transformIdx].position[0], 10.0f);
    ASSERT_FLOAT_EQ(tman.pTransformsCold[man.pDenseModelInstances[2].transformIdx].position[0], 20.0f);
    ASSERT_FLOAT_EQ(tman.pTransformsCold[man.pDenseModelInstances[3].transformIdx].position[0], 100.0f);
    ASSERT_FLOAT_EQ(tman.pTransformsCold[man.pDenseModelInstances[4].transformIdx].position[0], 101.0f);

    // Staging pointers should be NULLed
    ASSERT_NULL(man.pModel[0].pStagingInstances);
    ASSERT_NULL(man.pModel[1].pStagingInstances);
    ASSERT_NULL(man.pModel[0].pStagingTransforms);
    ASSERT_NULL(man.pModel[1].pStagingTransforms);

    // All instances alive
    for (uint32_t i = 0; i < 5; i++)
        ASSERT_EQ(man.pDenseModelInstances[i].lifecycle, INSTANCE_ALIVE);

    // Cleanup
    free(man.pDenseModelInstances);
    free(man.pModel);
    free(man.instanceOffset.items);
    free(tman.pTransformsHot);
    free(tman.pTransformsCold);
}

// ---------------------------------------------------------------------------
// 9. Staging consolidation with NULL staging (e.g. primitives)
// ---------------------------------------------------------------------------

void test_staging_consolidation_null_staging(void)
{
    // Simulate a model created by a primitive (pStagingInstances = NULL)
    Models3DManager man;
    memset(&man, 0, sizeof(man));

    TransformManager tman;
    memset(&tman, 0, sizeof(tman));

    man.modelLen = 1;
    man.pModel   = (Model3D*) calloc(1, sizeof(Model3D));

    man.pModel[0].instanceCount      = 2;
    man.pModel[0].alive              = MODEL_ALIVE;
    man.pModel[0].liveInstanceCount  = 0;
    man.pModel[0].pStagingInstances  = NULL; // like a primitive
    man.pModel[0].pStagingTransforms = NULL;

    man.modelInstanceCapacity = 2;

    fillModelInstancesIndices(&man, &tman);

    // Dense array allocated, but instances are zeroed (from calloc)
    ASSERT_NOT_NULL(man.pDenseModelInstances);
    // alive=0 means disabled (calloc zero-init)
    ASSERT_EQ(man.pDenseModelInstances[0].lifecycle, 0);
    ASSERT_EQ(man.pDenseModelInstances[1].lifecycle, 0);

    // Transforms should be identity-initialized
    ASSERT_EQ(tman.count, 2);

    free(man.pDenseModelInstances);
    free(man.pModel);
    free(man.instanceOffset.items);
    free(tman.pTransformsHot);
    free(tman.pTransformsCold);
}

// ---------------------------------------------------------------------------
// 10. Dense array integrity after destroy all instances then model
// ---------------------------------------------------------------------------

void test_destroy_all_instances_then_model(void)
{
    Models3DManager man = make_test_manager(1, 4);

    // Destroy every instance individually
    for (uint32_t i = 0; i < 4; i++)
        destroyInstance(&man, 0, i);

    // Then destroy the model
    destroyModel(&man, 0);

    ASSERT_EQ(man.pModel[0].alive, MODEL_DESTROYED);

    // Dense array still accessible (not freed), all instances destroyed
    for (uint32_t i = 0; i < 4; i++)
        ASSERT_EQ(man.pDenseModelInstances[i].lifecycle, INSTANCE_DESTROYED);

    // free_test_manager should not crash
    free_test_manager(&man);
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main(void)
{
    printf("=== Dense Array Tests ===\n");

    RUN_TEST(test_varied_instance_counts_offsets);
    RUN_TEST(test_varied_counts_lifecycle_operations);
    RUN_TEST(test_dense_boundary_no_cross_contamination);
    RUN_TEST(test_resolve_global_instance);
    RUN_TEST(test_resolve_global_instance_first_and_last);
    RUN_TEST(test_query_instance_alive_on_disabled_model);
    RUN_TEST(test_query_instance_alive_on_destroyed_model);
    RUN_TEST(test_ubo_fill_multi_frame_livecount_stability);
    RUN_TEST(test_destroy_model_then_ubo_fill);
    RUN_TEST(test_varied_counts_ubo_compaction);
    RUN_TEST(test_staging_consolidation);
    RUN_TEST(test_staging_consolidation_null_staging);
    RUN_TEST(test_destroy_all_instances_then_model);

    TEST_SUMMARY();
}
