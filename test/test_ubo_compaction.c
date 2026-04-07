#include "test_harness.h"
#include "test_helpers.h"
#include "uniformBuffer.h"
#include "transform.h"
#include "transformNode.h"

// ---------------------------------------------------------------------------
// Helpers — fake GraphicSwapchain with only currentExtent2D set
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
// Tests
// ---------------------------------------------------------------------------

void test_all_alive_full_output(void)
{
    SCENE*          scene = make_test_scene(2, 3);
    GraphicSwapchain sc   = make_fake_swapchain();
    Camera3D        cam   = make_test_camera();

    resolveTransforms(&scene->transformManager);
    MODEL_VIEW_PROJECTION* result = updateModelUniformBuffers(&sc, 0.0f, cam, scene);

    ASSERT_EQ(scene->modelsManager.modelInstanceLen, 6);
    ASSERT_EQ(scene->modelsManager.pModel[0].liveInstanceCount, 3);
    ASSERT_EQ(scene->modelsManager.pModel[1].liveInstanceCount, 3);
    ASSERT_EQ(scene->modelsManager.instanceOffset.items[0], 0);
    ASSERT_EQ(scene->modelsManager.instanceOffset.items[1], 3);

    free(result);
    free_test_scene(scene);
}

void test_one_instance_destroyed_compacted(void)
{
    SCENE*          scene = make_test_scene(1, 4);
    GraphicSwapchain sc   = make_fake_swapchain();
    Camera3D        cam   = make_test_camera();

    scene->modelsManager.pDenseModelInstances[scene->modelsManager.pModel[0].denseBaseOffset + 1].lifecycle = INSTANCE_DESTROYED;

    resolveTransforms(&scene->transformManager);
    MODEL_VIEW_PROJECTION* result = updateModelUniformBuffers(&sc, 0.0f, cam, scene);

    ASSERT_EQ(scene->modelsManager.pModel[0].liveInstanceCount, 3);
    ASSERT_EQ(scene->modelsManager.modelInstanceLen, 3);

    free(result);
    free_test_scene(scene);
}

void test_one_instance_disabled_compacted(void)
{
    SCENE*          scene = make_test_scene(1, 4);
    GraphicSwapchain sc   = make_fake_swapchain();
    Camera3D        cam   = make_test_camera();

    scene->modelsManager.pDenseModelInstances[scene->modelsManager.pModel[0].denseBaseOffset + 2].lifecycle = INSTANCE_DISABLED;

    resolveTransforms(&scene->transformManager);
    MODEL_VIEW_PROJECTION* result = updateModelUniformBuffers(&sc, 0.0f, cam, scene);

    ASSERT_EQ(scene->modelsManager.pModel[0].liveInstanceCount, 3);
    ASSERT_EQ(scene->modelsManager.modelInstanceLen, 3);

    free(result);
    free_test_scene(scene);
}

void test_all_instances_dead_zero_output(void)
{
    SCENE*          scene = make_test_scene(1, 4);
    GraphicSwapchain sc   = make_fake_swapchain();
    Camera3D        cam   = make_test_camera();

    for (uint32_t i = 0; i < 4; i++)
        scene->modelsManager.pDenseModelInstances[scene->modelsManager.pModel[0].denseBaseOffset + i].lifecycle = INSTANCE_DESTROYED;

    resolveTransforms(&scene->transformManager);
    MODEL_VIEW_PROJECTION* result = updateModelUniformBuffers(&sc, 0.0f, cam, scene);

    ASSERT_EQ(scene->modelsManager.pModel[0].liveInstanceCount, 0);
    ASSERT_EQ(scene->modelsManager.modelInstanceLen, 0);

    free(result);
    free_test_scene(scene);
}

void test_model_disabled_skips_all_instances(void)
{
    SCENE*          scene = make_test_scene(2, 3);
    GraphicSwapchain sc   = make_fake_swapchain();
    Camera3D        cam   = make_test_camera();

    scene->modelsManager.pModel[0].alive = MODEL_DISABLED;

    resolveTransforms(&scene->transformManager);
    MODEL_VIEW_PROJECTION* result = updateModelUniformBuffers(&sc, 0.0f, cam, scene);

    ASSERT_EQ(scene->modelsManager.pModel[0].liveInstanceCount, 0);
    ASSERT_EQ(scene->modelsManager.pModel[1].liveInstanceCount, 3);
    ASSERT_EQ(scene->modelsManager.modelInstanceLen, 3);
    ASSERT_EQ(scene->modelsManager.instanceOffset.items[0], 0);
    ASSERT_EQ(scene->modelsManager.instanceOffset.items[1], 0);

    free(result);
    free_test_scene(scene);
}

void test_model_destroyed_skips_all_instances(void)
{
    SCENE*          scene = make_test_scene(1, 4);
    GraphicSwapchain sc   = make_fake_swapchain();
    Camera3D        cam   = make_test_camera();

    scene->modelsManager.pModel[0].alive = MODEL_DESTROYED;

    resolveTransforms(&scene->transformManager);
    MODEL_VIEW_PROJECTION* result = updateModelUniformBuffers(&sc, 0.0f, cam, scene);

    ASSERT_EQ(scene->modelsManager.pModel[0].liveInstanceCount, 0);
    ASSERT_EQ(scene->modelsManager.modelInstanceLen, 0);

    free(result);
    free_test_scene(scene);
}

void test_mixed_state_multi_model(void)
{
    SCENE*          scene = make_test_scene(3, 4);
    GraphicSwapchain sc   = make_fake_swapchain();
    Camera3D        cam   = make_test_camera();

    // Model 0: kill instance 1 and 3 -> 2 live
    scene->modelsManager.pDenseModelInstances[scene->modelsManager.pModel[0].denseBaseOffset + 1].lifecycle = INSTANCE_DESTROYED;
    scene->modelsManager.pDenseModelInstances[scene->modelsManager.pModel[0].denseBaseOffset + 3].lifecycle = INSTANCE_DISABLED;
    // Model 1: disable entire model -> 0 live
    scene->modelsManager.pModel[1].alive = MODEL_DISABLED;
    // Model 2: all alive -> 4 live

    resolveTransforms(&scene->transformManager);
    MODEL_VIEW_PROJECTION* result = updateModelUniformBuffers(&sc, 0.0f, cam, scene);

    ASSERT_EQ(scene->modelsManager.pModel[0].liveInstanceCount, 2);
    ASSERT_EQ(scene->modelsManager.pModel[1].liveInstanceCount, 0);
    ASSERT_EQ(scene->modelsManager.pModel[2].liveInstanceCount, 4);
    ASSERT_EQ(scene->modelsManager.modelInstanceLen, 6);
    ASSERT_EQ(scene->modelsManager.instanceOffset.items[0], 0);
    ASSERT_EQ(scene->modelsManager.instanceOffset.items[1], 2);
    ASSERT_EQ(scene->modelsManager.instanceOffset.items[2], 2);

    free(result);
    free_test_scene(scene);
}

void test_compaction_preserves_correct_transforms(void)
{
    SCENE*          scene = make_test_scene(1, 3);
    GraphicSwapchain sc   = make_fake_swapchain();
    Camera3D        cam   = make_test_camera();

    // Set distinct positions so we can verify which survived compaction
    TransformManager* tman = &scene->transformManager;
    Model3DInstance*  inst0 = &scene->modelsManager.pDenseModelInstances[scene->modelsManager.pModel[0].denseBaseOffset + 0];
    Model3DInstance*  inst1 = &scene->modelsManager.pDenseModelInstances[scene->modelsManager.pModel[0].denseBaseOffset + 1];
    Model3DInstance*  inst2 = &scene->modelsManager.pDenseModelInstances[scene->modelsManager.pModel[0].denseBaseOffset + 2];

    transformSetPosition(&tman->pTransforms[inst0->transformIdx], (vec3){1.0f, 0.0f, 0.0f});
    transformSetPosition(&tman->pTransforms[inst1->transformIdx], (vec3){2.0f, 0.0f, 0.0f});
    transformSetPosition(&tman->pTransforms[inst2->transformIdx], (vec3){3.0f, 0.0f, 0.0f});

    // Kill instance 1
    inst1->lifecycle = INSTANCE_DESTROYED;

    resolveTransforms(tman);
    MODEL_VIEW_PROJECTION* result = updateModelUniformBuffers(&sc, 0.0f, cam, scene);

    // result[0] should have instance 0's model matrix (X translation = 1.0)
    // result[1] should have instance 2's model matrix (X translation = 3.0)
    ASSERT_FLOAT_EQ(result[0].model[3][0], 1.0f);
    ASSERT_FLOAT_EQ(result[1].model[3][0], 3.0f);
    ASSERT_EQ(scene->modelsManager.modelInstanceLen, 2);

    free(result);
    free_test_scene(scene);
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main(void)
{
    printf("=== UBO Compaction Tests ===\n");

    RUN_TEST(test_all_alive_full_output);
    RUN_TEST(test_one_instance_destroyed_compacted);
    RUN_TEST(test_one_instance_disabled_compacted);
    RUN_TEST(test_all_instances_dead_zero_output);
    RUN_TEST(test_model_disabled_skips_all_instances);
    RUN_TEST(test_model_destroyed_skips_all_instances);
    RUN_TEST(test_mixed_state_multi_model);
    RUN_TEST(test_compaction_preserves_correct_transforms);

    TEST_SUMMARY();
}
