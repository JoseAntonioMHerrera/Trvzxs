#include "test_harness.h"
#include "test_helpers.h"
#include "uniformBuffer.h"
#include "transform.h"

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
    cam.eye[0]    = 0.0f;
    cam.eye[1]    = 0.0f;
    cam.eye[2]    = 5.0f;
    cam.center[0] = 0.0f;
    cam.center[1] = 0.0f;
    cam.center[2] = 0.0f;
    cam.up[0]     = 0.0f;
    cam.up[1]     = 1.0f;
    cam.up[2]     = 0.0f;
    return cam;
}

// ---------------------------------------------------------------------------
// computeGlobalInstanceIndex tests
// ---------------------------------------------------------------------------

void test_global_index_single_model(void)
{
    Models3DManager man = make_test_manager(1, 5);

    ASSERT_EQ(computeGlobalInstanceIndex(&man.pModel[0], 0), 0);
    ASSERT_EQ(computeGlobalInstanceIndex(&man.pModel[0], 4), 4);

    free_test_manager(&man);
}

void test_global_index_multi_model(void)
{
    // 3 models with 4 instances each -> capacity offsets: 0, 4, 8
    Models3DManager man = make_test_manager(3, 4);

    ASSERT_EQ(computeGlobalInstanceIndex(&man.pModel[0], 0), 0);
    ASSERT_EQ(computeGlobalInstanceIndex(&man.pModel[0], 3), 3);
    ASSERT_EQ(computeGlobalInstanceIndex(&man.pModel[1], 0), 4);
    ASSERT_EQ(computeGlobalInstanceIndex(&man.pModel[1], 2), 6);
    ASSERT_EQ(computeGlobalInstanceIndex(&man.pModel[2], 0), 8);
    ASSERT_EQ(computeGlobalInstanceIndex(&man.pModel[2], 3), 11);

    free_test_manager(&man);
}

// ---------------------------------------------------------------------------
// pParentIndex initialization tests
// ---------------------------------------------------------------------------

void test_parent_index_init_all_roots(void)
{
    Models3DManager man = make_test_manager(2, 3);

    // All 6 slots should be INSTANCE_NO_PARENT
    for (uint32_t i = 0; i < man.modelInstanceCapacity; i++)
    {
        ASSERT_EQ(man.pParentIndex[i], INSTANCE_NO_PARENT);
    }

    free_test_manager(&man);
}

// ---------------------------------------------------------------------------
// instanceSetParent tests
// ---------------------------------------------------------------------------

void test_set_parent_basic(void)
{
    Models3DManager man = make_test_manager(1, 4);

    // Parent instance 0 of child instance 2 (same model)
    instanceSetParent(&man, 0, 2, 0, 0);

    uint32_t childGlobal  = computeGlobalInstanceIndex(&man.pModel[0], 2);
    uint32_t parentGlobal = computeGlobalInstanceIndex(&man.pModel[0], 0);
    ASSERT_EQ(man.pParentIndex[childGlobal], parentGlobal);

    // Other instances should remain roots
    ASSERT_EQ(man.pParentIndex[computeGlobalInstanceIndex(&man.pModel[0], 0)], INSTANCE_NO_PARENT);
    ASSERT_EQ(man.pParentIndex[computeGlobalInstanceIndex(&man.pModel[0], 1)], INSTANCE_NO_PARENT);
    ASSERT_EQ(man.pParentIndex[computeGlobalInstanceIndex(&man.pModel[0], 3)], INSTANCE_NO_PARENT);

    free_test_manager(&man);
}

void test_set_parent_cross_model(void)
{
    // 2 models, 3 instances each -> global indices: model0=[0,1,2], model1=[3,4,5]
    Models3DManager man = make_test_manager(2, 3);

    // Instance 1 of model 1 (global=4) parented to instance 2 of model 0 (global=2)
    instanceSetParent(&man, 1, 1, 0, 2);

    ASSERT_EQ(man.pParentIndex[4], 2);

    free_test_manager(&man);
}

void test_set_parent_rejects_invalid_order(void)
{
    Models3DManager man = make_test_manager(1, 4);

    // Try to parent instance 0 under instance 2 — invalid because parent global > child global
    CAPTURE_STDERR_BEGIN();
    instanceSetParent(&man, 0, 0, 0, 2);
    CAPTURE_STDERR_END();

    // Should be rejected — instance 0 remains a root
    ASSERT_EQ(man.pParentIndex[0], INSTANCE_NO_PARENT);
    ASSERT_STDERR_CONTAINS("parent global index");

    free_test_manager(&man);
}

void test_set_parent_rejects_self_parent(void)
{
    Models3DManager man = make_test_manager(1, 4);

    // Try to parent instance 1 under itself
    CAPTURE_STDERR_BEGIN();
    instanceSetParent(&man, 0, 1, 0, 1);
    CAPTURE_STDERR_END();

    // Should be rejected (parentGlobal == childGlobal, not strictly less)
    ASSERT_EQ(man.pParentIndex[1], INSTANCE_NO_PARENT);
    ASSERT_STDERR_CONTAINS("parent global index");

    free_test_manager(&man);
}

// ---------------------------------------------------------------------------
// UBO fill with hierarchy tests
// ---------------------------------------------------------------------------

void test_ubo_root_instances_unchanged(void)
{
    // Verify that instances with no parent produce the same transforms as before
    SCENE*          scene = make_test_scene(1, 3);
    GraphicSwapchain sc   = make_fake_swapchain();
    Camera3D        cam   = make_test_camera();

    instanceSetPosition(&scene->modelsManager.pDenseModelInstances[scene->modelsManager.pModel[0].denseBaseOffset + 0], (vec3){1.0f, 0.0f, 0.0f});
    instanceSetPosition(&scene->modelsManager.pDenseModelInstances[scene->modelsManager.pModel[0].denseBaseOffset + 1], (vec3){2.0f, 0.0f, 0.0f});
    instanceSetPosition(&scene->modelsManager.pDenseModelInstances[scene->modelsManager.pModel[0].denseBaseOffset + 2], (vec3){3.0f, 0.0f, 0.0f});

    MODEL_VIEW_PROJECTION* result = updateModelUniformBuffers(&sc, 0.0f, cam, scene);

    // All roots — transforms should be pure local TRS
    ASSERT_FLOAT_EQ(result[0].model[3][0], 1.0f);
    ASSERT_FLOAT_EQ(result[1].model[3][0], 2.0f);
    ASSERT_FLOAT_EQ(result[2].model[3][0], 3.0f);

    free(result);
    free_test_scene(scene);
}

void test_ubo_child_inherits_parent_translation(void)
{
    // Parent at X=10, child at X=5 local -> child world should be X=15
    SCENE*          scene = make_test_scene(1, 3);
    GraphicSwapchain sc   = make_fake_swapchain();
    Camera3D        cam   = make_test_camera();

    // Instance 0 = parent at X=10
    instanceSetPosition(&scene->modelsManager.pDenseModelInstances[scene->modelsManager.pModel[0].denseBaseOffset + 0], (vec3){10.0f, 0.0f, 0.0f});
    // Instance 1 = child with local X=5
    instanceSetPosition(&scene->modelsManager.pDenseModelInstances[scene->modelsManager.pModel[0].denseBaseOffset + 1], (vec3){5.0f, 0.0f, 0.0f});
    // Instance 2 = root at X=20
    instanceSetPosition(&scene->modelsManager.pDenseModelInstances[scene->modelsManager.pModel[0].denseBaseOffset + 2], (vec3){20.0f, 0.0f, 0.0f});

    // Parent instance 1 under instance 0
    instanceSetParent(&scene->modelsManager, 0, 1, 0, 0);

    MODEL_VIEW_PROJECTION* result = updateModelUniformBuffers(&sc, 0.0f, cam, scene);

    // Instance 0 (root): world X = 10
    ASSERT_FLOAT_EQ(result[0].model[3][0], 10.0f);
    // Instance 1 (child of 0): world X = parent(10) + local(5) = 15
    ASSERT_FLOAT_EQ(result[1].model[3][0], 15.0f);
    // Instance 2 (root): world X = 20
    ASSERT_FLOAT_EQ(result[2].model[3][0], 20.0f);

    free(result);
    free_test_scene(scene);
}

void test_ubo_grandchild_chain(void)
{
    // 3-level chain: instance 0 -> instance 1 -> instance 2
    // All at X=5 local -> world should be 5, 10, 15
    SCENE*          scene = make_test_scene(1, 3);
    GraphicSwapchain sc   = make_fake_swapchain();
    Camera3D        cam   = make_test_camera();

    instanceSetPosition(&scene->modelsManager.pDenseModelInstances[scene->modelsManager.pModel[0].denseBaseOffset + 0], (vec3){5.0f, 0.0f, 0.0f});
    instanceSetPosition(&scene->modelsManager.pDenseModelInstances[scene->modelsManager.pModel[0].denseBaseOffset + 1], (vec3){5.0f, 0.0f, 0.0f});
    instanceSetPosition(&scene->modelsManager.pDenseModelInstances[scene->modelsManager.pModel[0].denseBaseOffset + 2], (vec3){5.0f, 0.0f, 0.0f});

    instanceSetParent(&scene->modelsManager, 0, 1, 0, 0); // 1 child of 0
    instanceSetParent(&scene->modelsManager, 0, 2, 0, 1); // 2 child of 1

    MODEL_VIEW_PROJECTION* result = updateModelUniformBuffers(&sc, 0.0f, cam, scene);

    // Root: 5
    ASSERT_FLOAT_EQ(result[0].model[3][0], 5.0f);
    // Child: 5 + 5 = 10
    ASSERT_FLOAT_EQ(result[1].model[3][0], 10.0f);
    // Grandchild: 10 + 5 = 15
    ASSERT_FLOAT_EQ(result[2].model[3][0], 15.0f);

    free(result);
    free_test_scene(scene);
}

void test_ubo_cross_model_parenting(void)
{
    // 2 models, 2 instances each
    // Model 0 instance 0 (global=0) is parent of model 1 instance 0 (global=2)
    SCENE*          scene = make_test_scene(2, 2);
    GraphicSwapchain sc   = make_fake_swapchain();
    Camera3D        cam   = make_test_camera();

    instanceSetPosition(&scene->modelsManager.pDenseModelInstances[scene->modelsManager.pModel[0].denseBaseOffset + 0], (vec3){10.0f, 0.0f, 0.0f});
    instanceSetPosition(&scene->modelsManager.pDenseModelInstances[scene->modelsManager.pModel[0].denseBaseOffset + 1], (vec3){0.0f, 0.0f, 0.0f});
    instanceSetPosition(&scene->modelsManager.pDenseModelInstances[scene->modelsManager.pModel[1].denseBaseOffset + 0], (vec3){3.0f, 0.0f, 0.0f});
    instanceSetPosition(&scene->modelsManager.pDenseModelInstances[scene->modelsManager.pModel[1].denseBaseOffset + 1], (vec3){0.0f, 0.0f, 0.0f});

    // Model 1, instance 0 parented to model 0, instance 0
    instanceSetParent(&scene->modelsManager, 1, 0, 0, 0);

    MODEL_VIEW_PROJECTION* result = updateModelUniformBuffers(&sc, 0.0f, cam, scene);

    // Model 0 inst 0 (root): X=10
    ASSERT_FLOAT_EQ(result[0].model[3][0], 10.0f);
    // Model 0 inst 1 (root): X=0
    ASSERT_FLOAT_EQ(result[1].model[3][0], 0.0f);
    // Model 1 inst 0 (child of model0/inst0): X = 10 + 3 = 13
    ASSERT_FLOAT_EQ(result[2].model[3][0], 13.0f);
    // Model 1 inst 1 (root): X=0
    ASSERT_FLOAT_EQ(result[3].model[3][0], 0.0f);

    free(result);
    free_test_scene(scene);
}

void test_ubo_multiple_children_same_parent(void)
{
    // Instance 0 is parent of instances 1, 2, and 3
    SCENE*          scene = make_test_scene(1, 4);
    GraphicSwapchain sc   = make_fake_swapchain();
    Camera3D        cam   = make_test_camera();

    instanceSetPosition(&scene->modelsManager.pDenseModelInstances[scene->modelsManager.pModel[0].denseBaseOffset + 0], (vec3){10.0f, 0.0f, 0.0f});
    instanceSetPosition(&scene->modelsManager.pDenseModelInstances[scene->modelsManager.pModel[0].denseBaseOffset + 1], (vec3){1.0f, 0.0f, 0.0f});
    instanceSetPosition(&scene->modelsManager.pDenseModelInstances[scene->modelsManager.pModel[0].denseBaseOffset + 2], (vec3){2.0f, 0.0f, 0.0f});
    instanceSetPosition(&scene->modelsManager.pDenseModelInstances[scene->modelsManager.pModel[0].denseBaseOffset + 3], (vec3){3.0f, 0.0f, 0.0f});

    instanceSetParent(&scene->modelsManager, 0, 1, 0, 0);
    instanceSetParent(&scene->modelsManager, 0, 2, 0, 0);
    instanceSetParent(&scene->modelsManager, 0, 3, 0, 0);

    MODEL_VIEW_PROJECTION* result = updateModelUniformBuffers(&sc, 0.0f, cam, scene);

    ASSERT_FLOAT_EQ(result[0].model[3][0], 10.0f);       // root
    ASSERT_FLOAT_EQ(result[1].model[3][0], 11.0f);       // 10 + 1
    ASSERT_FLOAT_EQ(result[2].model[3][0], 12.0f);       // 10 + 2
    ASSERT_FLOAT_EQ(result[3].model[3][0], 13.0f);       // 10 + 3

    free(result);
    free_test_scene(scene);
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main(void)
{
    printf("=== Instance Hierarchy Tests ===\n");

    RUN_TEST(test_global_index_single_model);
    RUN_TEST(test_global_index_multi_model);
    RUN_TEST(test_parent_index_init_all_roots);
    RUN_TEST(test_set_parent_basic);
    RUN_TEST(test_set_parent_cross_model);
    RUN_TEST(test_set_parent_rejects_invalid_order);
    RUN_TEST(test_set_parent_rejects_self_parent);
    RUN_TEST(test_ubo_root_instances_unchanged);
    RUN_TEST(test_ubo_child_inherits_parent_translation);
    RUN_TEST(test_ubo_grandchild_chain);
    RUN_TEST(test_ubo_cross_model_parenting);
    RUN_TEST(test_ubo_multiple_children_same_parent);

    TEST_SUMMARY();
}
