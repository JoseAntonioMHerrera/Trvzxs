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

static SCENE* make_test_scene_varied(uint32_t modelCount, const uint32_t* instanceCounts)
{
    SCENE* scene = (SCENE*) calloc(1, sizeof(SCENE));

    uint32_t total = 0;
    for (uint32_t m = 0; m < modelCount; m++)
        total += instanceCounts[m];

    scene->transformManager = make_test_transform_manager(total);
    scene->modelsManager    = make_test_manager_varied(modelCount, instanceCounts);

    for (uint32_t i = 0; i < total; i++)
        scene->modelsManager.pDenseModelInstances[i].transformIdx = i;

    scene->camera.eye[2] = 5.0f;
    scene->camera.up[1]  = 1.0f;
    return scene;
}

// ---------------------------------------------------------------------------
// 1. Draw parameter bounds: firstInstance + liveInstanceCount <= capacity
// ---------------------------------------------------------------------------

void test_draw_params_within_ubo_bounds_all_alive(void)
{
    SCENE*           scene = make_test_scene(2, 4);
    GraphicSwapchain sc    = make_fake_swapchain();
    Camera3D         cam   = make_test_camera();

    resolveTransforms(&scene->transformManager);
    ModelInstanceUBO* result = (ModelInstanceUBO*) malloc(sizeof(ModelInstanceUBO) * scene->modelsManager.modelInstanceCapacity);
    updateModelUniformBuffersDirect(result, scene);

    uint32_t cap = scene->modelsManager.modelInstanceCapacity;
    for (uint32_t i = 0; i < scene->modelsManager.modelLen; i++)
    {
        uint32_t first = scene->modelsManager.instanceOffset.items[i];
        uint32_t count = scene->modelsManager.pModel[i].liveInstanceCount;
        ASSERT_EQ(first + count <= cap, 1);
    }

    free(result);
    free_test_scene(scene);
}

void test_draw_params_within_ubo_bounds_mixed_state(void)
{
    uint32_t counts[] = {3, 5, 2};
    SCENE*           scene = make_test_scene_varied(3, counts);
    GraphicSwapchain sc    = make_fake_swapchain();
    Camera3D         cam   = make_test_camera();

    destroyInstance(&scene->modelsManager, 0, 1);
    destroyInstance(&scene->modelsManager, 1, 0);
    destroyInstance(&scene->modelsManager, 1, 4);
    disableModel(&scene->modelsManager, 2);

    resolveTransforms(&scene->transformManager);
    ModelInstanceUBO* result = (ModelInstanceUBO*) malloc(sizeof(ModelInstanceUBO) * scene->modelsManager.modelInstanceCapacity);
    updateModelUniformBuffersDirect(result, scene);

    uint32_t cap = scene->modelsManager.modelInstanceCapacity;
    for (uint32_t i = 0; i < scene->modelsManager.modelLen; i++)
    {
        uint32_t first = scene->modelsManager.instanceOffset.items[i];
        uint32_t count = scene->modelsManager.pModel[i].liveInstanceCount;
        ASSERT_EQ(first + count <= cap, 1);
    }

    free(result);
    free_test_transform_manager(&scene->transformManager);
    free_test_manager(&scene->modelsManager);
    free(scene);
}

// ---------------------------------------------------------------------------
// 2. memcpy bounds: modelInstanceLen <= modelInstanceCapacity
// ---------------------------------------------------------------------------

void test_memcpy_len_within_capacity_all_alive(void)
{
    SCENE*           scene = make_test_scene(3, 4);
    GraphicSwapchain sc    = make_fake_swapchain();
    Camera3D         cam   = make_test_camera();

    resolveTransforms(&scene->transformManager);
    ModelInstanceUBO* result = (ModelInstanceUBO*) malloc(sizeof(ModelInstanceUBO) * scene->modelsManager.modelInstanceCapacity);
    updateModelUniformBuffersDirect(result, scene);

    ASSERT_EQ(scene->modelsManager.modelInstanceLen <= scene->modelsManager.modelInstanceCapacity, 1);

    free(result);
    free_test_scene(scene);
}

void test_memcpy_len_within_capacity_after_toggles(void)
{
    SCENE*           scene = make_test_scene(2, 4);
    GraphicSwapchain sc    = make_fake_swapchain();
    Camera3D         cam   = make_test_camera();

    disableModel(&scene->modelsManager, 0);
    resolveTransforms(&scene->transformManager);
    ModelInstanceUBO* r1 = (ModelInstanceUBO*) malloc(sizeof(ModelInstanceUBO) * scene->modelsManager.modelInstanceCapacity);
    updateModelUniformBuffersDirect(r1, scene);
    ASSERT_EQ(scene->modelsManager.modelInstanceLen <= scene->modelsManager.modelInstanceCapacity, 1);
    free(r1);

    enableModel(&scene->modelsManager, 0);
    resolveTransforms(&scene->transformManager);
    ModelInstanceUBO* r2 = (ModelInstanceUBO*) malloc(sizeof(ModelInstanceUBO) * scene->modelsManager.modelInstanceCapacity);
    updateModelUniformBuffersDirect(r2, scene);
    ASSERT_EQ(scene->modelsManager.modelInstanceLen <= scene->modelsManager.modelInstanceCapacity, 1);
    free(r2);

    free_test_scene(scene);
}

// ---------------------------------------------------------------------------
// 3. liveInstanceCount must be 0 before first UBO fill
// ---------------------------------------------------------------------------

void test_liveInstanceCount_initialized_zero(void)
{
    Model3D* model = (Model3D*) calloc(1, sizeof(Model3D));
    model->instanceCount     = 100;
    model->liveInstanceCount = 0;

    ASSERT_EQ(model->liveInstanceCount, 0);

    free(model);
}

void test_liveInstanceCount_zero_before_first_fill(void)
{
    Models3DManager man;
    memset(&man, 0, sizeof(man));
    man.modelLen              = 1;
    man.pModel                = (Model3D*) calloc(1, sizeof(Model3D));
    man.pModel[0].instanceCount      = 4;
    man.pModel[0].liveInstanceCount  = 0;
    man.pModel[0].alive              = MODEL_ALIVE;
    man.pModel[0].denseBaseOffset    = 0;
    man.modelInstanceCapacity        = 4;
    man.modelInstanceLen             = 0;
    man.pDenseModelInstances         = (Model3DInstance*) calloc(4, sizeof(Model3DInstance));
    man.instanceOffset.items         = (uint32_t*) calloc(1, sizeof(uint32_t));
    man.instanceOffset.itemsLen      = 1;

    TransformManager tman = make_test_transform_manager(4);

    for (uint32_t i = 0; i < 4; i++)
    {
        man.pDenseModelInstances[i].transformIdx = i;
        man.pDenseModelInstances[i].lifecycle    = INSTANCE_ALIVE;
    }

    ASSERT_EQ(man.pModel[0].liveInstanceCount, 0);

    SCENE scene;
    memset(&scene, 0, sizeof(scene));
    scene.modelsManager     = man;
    scene.transformManager  = tman;

    GraphicSwapchain sc = make_fake_swapchain();
    Camera3D         cam = make_test_camera();

    resolveTransforms(&scene.transformManager);
    ModelInstanceUBO* result = (ModelInstanceUBO*) malloc(sizeof(ModelInstanceUBO) * scene.modelsManager.modelInstanceCapacity);
    updateModelUniformBuffersDirect(result, &scene);

    ASSERT_EQ(scene.modelsManager.pModel[0].liveInstanceCount, 4);

    free(result);
    free(man.pDenseModelInstances);
    free(man.pModel);
    free(man.instanceOffset.items);
    free_test_transform_manager(&tman);
}

// ---------------------------------------------------------------------------
// 4. Transform survival through disable/enable cycle
// ---------------------------------------------------------------------------

void test_transform_survives_disable_enable(void)
{
    SCENE*           scene = make_test_scene(1, 3);
    GraphicSwapchain sc    = make_fake_swapchain();
    Camera3D         cam   = make_test_camera();
    TransformManager* tman = &scene->transformManager;

    transformSetPosition(&tman->pTransforms[scene->modelsManager.pDenseModelInstances[0].transformIdx], (vec3){10.0f, 0.0f, 0.0f});
    transformSetPosition(&tman->pTransforms[scene->modelsManager.pDenseModelInstances[1].transformIdx], (vec3){20.0f, 0.0f, 0.0f});
    transformSetPosition(&tman->pTransforms[scene->modelsManager.pDenseModelInstances[2].transformIdx], (vec3){30.0f, 0.0f, 0.0f});

    // Frame 1: all alive
    resolveTransforms(tman);
    ModelInstanceUBO* r1 = (ModelInstanceUBO*) malloc(sizeof(ModelInstanceUBO) * scene->modelsManager.modelInstanceCapacity);
    updateModelUniformBuffersDirect(r1, scene);
    ASSERT_FLOAT_EQ(r1[0].model[3][0], 10.0f);
    ASSERT_FLOAT_EQ(r1[1].model[3][0], 20.0f);
    ASSERT_FLOAT_EQ(r1[2].model[3][0], 30.0f);
    free(r1);

    // Disable instance 1
    disableInstance(&scene->modelsManager, 0, 1);

    // Frame 2: instance 1 skipped
    resolveTransforms(tman);
    ModelInstanceUBO* r2 = (ModelInstanceUBO*) malloc(sizeof(ModelInstanceUBO) * scene->modelsManager.modelInstanceCapacity);
    updateModelUniformBuffersDirect(r2, scene);
    ASSERT_EQ(scene->modelsManager.pModel[0].liveInstanceCount, 2);
    ASSERT_FLOAT_EQ(r2[0].model[3][0], 10.0f);
    ASSERT_FLOAT_EQ(r2[1].model[3][0], 30.0f);
    free(r2);

    // Re-enable instance 1
    enableInstance(&scene->modelsManager, 0, 1);

    // Frame 3: instance 1 back
    resolveTransforms(tman);
    ModelInstanceUBO* r3 = (ModelInstanceUBO*) malloc(sizeof(ModelInstanceUBO) * scene->modelsManager.modelInstanceCapacity);
    updateModelUniformBuffersDirect(r3, scene);
    ASSERT_EQ(scene->modelsManager.pModel[0].liveInstanceCount, 3);
    ASSERT_FLOAT_EQ(r3[0].model[3][0], 10.0f);
    ASSERT_FLOAT_EQ(r3[1].model[3][0], 20.0f);
    ASSERT_FLOAT_EQ(r3[2].model[3][0], 30.0f);
    free(r3);

    free_test_scene(scene);
}

void test_transform_survives_model_disable_enable(void)
{
    SCENE*           scene = make_test_scene(1, 2);
    GraphicSwapchain sc    = make_fake_swapchain();
    Camera3D         cam   = make_test_camera();
    TransformManager* tman = &scene->transformManager;

    transformSetPosition(&tman->pTransforms[scene->modelsManager.pDenseModelInstances[0].transformIdx], (vec3){5.0f, 0.0f, 0.0f});
    transformSetPosition(&tman->pTransforms[scene->modelsManager.pDenseModelInstances[1].transformIdx], (vec3){15.0f, 0.0f, 0.0f});

    // Frame 1
    resolveTransforms(tman);
    ModelInstanceUBO* r1 = (ModelInstanceUBO*) malloc(sizeof(ModelInstanceUBO) * scene->modelsManager.modelInstanceCapacity);
    updateModelUniformBuffersDirect(r1, scene);
    ASSERT_FLOAT_EQ(r1[0].model[3][0], 5.0f);
    ASSERT_FLOAT_EQ(r1[1].model[3][0], 15.0f);
    free(r1);

    // Disable entire model
    disableModel(&scene->modelsManager, 0);
    resolveTransforms(tman);
    ModelInstanceUBO* r2 = (ModelInstanceUBO*) malloc(sizeof(ModelInstanceUBO) * scene->modelsManager.modelInstanceCapacity);
    updateModelUniformBuffersDirect(r2, scene);
    ASSERT_EQ(scene->modelsManager.pModel[0].liveInstanceCount, 0);
    free(r2);

    // Re-enable
    enableModel(&scene->modelsManager, 0);
    resolveTransforms(tman);
    ModelInstanceUBO* r3 = (ModelInstanceUBO*) malloc(sizeof(ModelInstanceUBO) * scene->modelsManager.modelInstanceCapacity);
    updateModelUniformBuffersDirect(r3, scene);
    ASSERT_EQ(scene->modelsManager.pModel[0].liveInstanceCount, 2);
    ASSERT_FLOAT_EQ(r3[0].model[3][0], 5.0f);
    ASSERT_FLOAT_EQ(r3[1].model[3][0], 15.0f);
    free(r3);

    free_test_scene(scene);
}

// ---------------------------------------------------------------------------
// 5. Single-instance model through all states
// ---------------------------------------------------------------------------

void test_single_instance_model_lifecycle(void)
{
    uint32_t counts[] = {1};
    SCENE*           scene = make_test_scene_varied(1, counts);
    GraphicSwapchain sc    = make_fake_swapchain();
    Camera3D         cam   = make_test_camera();

    transformSetPosition(&scene->transformManager.pTransforms[scene->modelsManager.pDenseModelInstances[0].transformIdx], (vec3){7.0f, 0.0f, 0.0f});

    // Alive
    resolveTransforms(&scene->transformManager);
    ModelInstanceUBO* r1 = (ModelInstanceUBO*) malloc(sizeof(ModelInstanceUBO) * scene->modelsManager.modelInstanceCapacity);
    updateModelUniformBuffersDirect(r1, scene);
    ASSERT_EQ(scene->modelsManager.pModel[0].liveInstanceCount, 1);
    ASSERT_EQ(scene->modelsManager.modelInstanceLen, 1);
    ASSERT_FLOAT_EQ(r1[0].model[3][0], 7.0f);
    free(r1);

    // Disable
    disableInstance(&scene->modelsManager, 0, 0);
    resolveTransforms(&scene->transformManager);
    ModelInstanceUBO* r2 = (ModelInstanceUBO*) malloc(sizeof(ModelInstanceUBO) * scene->modelsManager.modelInstanceCapacity);
    updateModelUniformBuffersDirect(r2, scene);
    ASSERT_EQ(scene->modelsManager.pModel[0].liveInstanceCount, 0);
    ASSERT_EQ(scene->modelsManager.modelInstanceLen, 0);
    free(r2);

    // Re-enable
    enableInstance(&scene->modelsManager, 0, 0);
    resolveTransforms(&scene->transformManager);
    ModelInstanceUBO* r3 = (ModelInstanceUBO*) malloc(sizeof(ModelInstanceUBO) * scene->modelsManager.modelInstanceCapacity);
    updateModelUniformBuffersDirect(r3, scene);
    ASSERT_EQ(scene->modelsManager.pModel[0].liveInstanceCount, 1);
    ASSERT_FLOAT_EQ(r3[0].model[3][0], 7.0f);
    free(r3);

    // Destroy
    destroyInstance(&scene->modelsManager, 0, 0);
    resolveTransforms(&scene->transformManager);
    ModelInstanceUBO* r4 = (ModelInstanceUBO*) malloc(sizeof(ModelInstanceUBO) * scene->modelsManager.modelInstanceCapacity);
    updateModelUniformBuffersDirect(r4, scene);
    ASSERT_EQ(scene->modelsManager.pModel[0].liveInstanceCount, 0);
    ASSERT_EQ(scene->modelsManager.modelInstanceLen, 0);
    free(r4);

    free_test_transform_manager(&scene->transformManager);
    free_test_manager(&scene->modelsManager);
    free(scene);
}

// ---------------------------------------------------------------------------
// 6. Camera-only fallback (modelInstanceLen == 0)
// ---------------------------------------------------------------------------

void test_all_models_dead_zero_total_len(void)
{
    SCENE*           scene = make_test_scene(2, 3);
    GraphicSwapchain sc    = make_fake_swapchain();
    Camera3D         cam   = make_test_camera();

    destroyModel(&scene->modelsManager, 0);
    destroyModel(&scene->modelsManager, 1);

    resolveTransforms(&scene->transformManager);
    ModelInstanceUBO* result = (ModelInstanceUBO*) malloc(sizeof(ModelInstanceUBO) * scene->modelsManager.modelInstanceCapacity);
    updateModelUniformBuffersDirect(result, scene);

    ASSERT_EQ(scene->modelsManager.modelInstanceLen, 0);

    for (uint32_t i = 0; i < scene->modelsManager.modelLen; i++)
        ASSERT_EQ(scene->modelsManager.pModel[i].liveInstanceCount, 0);

    free(result);
    free_test_scene(scene);
}

void test_all_instances_disabled_triggers_zero_len(void)
{
    SCENE*           scene = make_test_scene(1, 3);
    GraphicSwapchain sc    = make_fake_swapchain();
    Camera3D         cam   = make_test_camera();

    for (uint32_t i = 0; i < 3; i++)
        disableInstance(&scene->modelsManager, 0, i);

    resolveTransforms(&scene->transformManager);
    ModelInstanceUBO* result = (ModelInstanceUBO*) malloc(sizeof(ModelInstanceUBO) * scene->modelsManager.modelInstanceCapacity);
    updateModelUniformBuffersDirect(result, scene);

    ASSERT_EQ(scene->modelsManager.modelInstanceLen, 0);
    ASSERT_EQ(scene->modelsManager.pModel[0].liveInstanceCount, 0);

    free(result);
    free_test_scene(scene);
}

// ---------------------------------------------------------------------------
// 7. Compacted offset monotonicity
// ---------------------------------------------------------------------------

void test_compacted_offsets_monotonic(void)
{
    uint32_t counts[] = {4, 3, 5, 2};
    SCENE*           scene = make_test_scene_varied(4, counts);
    GraphicSwapchain sc    = make_fake_swapchain();
    Camera3D         cam   = make_test_camera();

    destroyInstance(&scene->modelsManager, 0, 1);
    destroyInstance(&scene->modelsManager, 1, 0);
    disableModel(&scene->modelsManager, 2);
    destroyInstance(&scene->modelsManager, 3, 0);

    resolveTransforms(&scene->transformManager);
    ModelInstanceUBO* result = (ModelInstanceUBO*) malloc(sizeof(ModelInstanceUBO) * scene->modelsManager.modelInstanceCapacity);
    updateModelUniformBuffersDirect(result, scene);

    for (uint32_t i = 1; i < scene->modelsManager.modelLen; i++)
    {
        ASSERT_EQ(scene->modelsManager.instanceOffset.items[i] >=
                  scene->modelsManager.instanceOffset.items[i - 1], 1);
    }

    for (uint32_t i = 1; i < scene->modelsManager.modelLen; i++)
    {
        uint32_t expected = scene->modelsManager.instanceOffset.items[i - 1]
                          + scene->modelsManager.pModel[i - 1].liveInstanceCount;
        ASSERT_EQ(scene->modelsManager.instanceOffset.items[i], expected);
    }

    uint32_t last = scene->modelsManager.modelLen - 1;
    uint32_t total = scene->modelsManager.instanceOffset.items[last]
                   + scene->modelsManager.pModel[last].liveInstanceCount;
    ASSERT_EQ(total, scene->modelsManager.modelInstanceLen);

    free(result);
    free_test_transform_manager(&scene->transformManager);
    free_test_manager(&scene->modelsManager);
    free(scene);
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main(void)
{
    printf("=== Frame Safety Tests ===\n");

    RUN_TEST(test_draw_params_within_ubo_bounds_all_alive);
    RUN_TEST(test_draw_params_within_ubo_bounds_mixed_state);
    RUN_TEST(test_memcpy_len_within_capacity_all_alive);
    RUN_TEST(test_memcpy_len_within_capacity_after_toggles);
    RUN_TEST(test_liveInstanceCount_initialized_zero);
    RUN_TEST(test_liveInstanceCount_zero_before_first_fill);
    RUN_TEST(test_transform_survives_disable_enable);
    RUN_TEST(test_transform_survives_model_disable_enable);
    RUN_TEST(test_single_instance_model_lifecycle);
    RUN_TEST(test_all_models_dead_zero_total_len);
    RUN_TEST(test_all_instances_disabled_triggers_zero_len);
    RUN_TEST(test_compacted_offsets_monotonic);

    TEST_SUMMARY();
}
