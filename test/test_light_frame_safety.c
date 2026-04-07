#include "test_harness.h"
#include "test_helpers.h"
#include "uniformBuffer.h"
#include "light.h"
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

static SCENE* make_light_scene(uint32_t lightCount, uint32_t instancesPerLight)
{
    SCENE* scene = (SCENE*) calloc(1, sizeof(SCENE));
    scene->lightManager = make_test_light_manager(lightCount, instancesPerLight);
    scene->camera.eye[2] = 5.0f;
    scene->camera.up[1]  = 1.0f;
    return scene;
}

static void free_light_scene(SCENE* scene)
{
    if (!scene) return;
    free_test_light_manager(&scene->lightManager);
    free(scene);
}

// ---------------------------------------------------------------------------
// 1. Draw parameter bounds: firstInstance + liveInstanceCount <= capacity
//
//    executeLightPass does:
//      vkCmdDraw(..., liveInstanceCount, ..., instanceOffset.items[i])
//    GPU reads UBO at [firstInstance .. firstInstance + liveInstanceCount).
// ---------------------------------------------------------------------------

void test_light_draw_params_within_bounds_all_alive(void)
{
    SCENE*           scene = make_light_scene(2, 4);
    GraphicSwapchain sc    = make_fake_swapchain();
    Camera3D         cam   = make_test_camera();

    LightTransformations* result = updateLightUniformBuffers(&sc, 0.0f, cam, scene);

    uint32_t cap = scene->lightManager.lightInstanceCapacity;
    for (uint32_t i = 0; i < scene->lightManager.lightLen; i++)
    {
        uint32_t first = scene->lightManager.instanceOffset.items[i];
        uint32_t count = scene->lightManager.pLight[i].liveInstanceCount;
        ASSERT_EQ(first + count <= cap, 1);
    }

    free(result);
    free_light_scene(scene);
}

void test_light_draw_params_within_bounds_mixed_state(void)
{
    SCENE*           scene = make_light_scene(3, 4);
    GraphicSwapchain sc    = make_fake_swapchain();
    Camera3D         cam   = make_test_camera();

    destroyLightInstance(&scene->lightManager, 0, 1);
    destroyLightInstance(&scene->lightManager, 1, 0);
    disableLight(&scene->lightManager, 2);

    LightTransformations* result = updateLightUniformBuffers(&sc, 0.0f, cam, scene);

    uint32_t cap = scene->lightManager.lightInstanceCapacity;
    for (uint32_t i = 0; i < scene->lightManager.lightLen; i++)
    {
        uint32_t first = scene->lightManager.instanceOffset.items[i];
        uint32_t count = scene->lightManager.pLight[i].liveInstanceCount;
        ASSERT_EQ(first + count <= cap, 1);
    }

    free(result);
    free_light_scene(scene);
}

// ---------------------------------------------------------------------------
// 2. memcpy bounds: lightInstanceCount <= lightInstanceCapacity
// ---------------------------------------------------------------------------

void test_light_memcpy_len_within_capacity(void)
{
    SCENE*           scene = make_light_scene(3, 4);
    GraphicSwapchain sc    = make_fake_swapchain();
    Camera3D         cam   = make_test_camera();

    LightTransformations* r1 = updateLightUniformBuffers(&sc, 0.0f, cam, scene);
    ASSERT_EQ(scene->lightManager.lightInstanceCount <= scene->lightManager.lightInstanceCapacity, 1);
    free(r1);

    // After toggles
    disableLight(&scene->lightManager, 0);
    LightTransformations* r2 = updateLightUniformBuffers(&sc, 0.016f, cam, scene);
    ASSERT_EQ(scene->lightManager.lightInstanceCount <= scene->lightManager.lightInstanceCapacity, 1);
    free(r2);

    enableLight(&scene->lightManager, 0);
    LightTransformations* r3 = updateLightUniformBuffers(&sc, 0.032f, cam, scene);
    ASSERT_EQ(scene->lightManager.lightInstanceCount <= scene->lightManager.lightInstanceCapacity, 1);
    free(r3);

    free_light_scene(scene);
}

// ---------------------------------------------------------------------------
// 3. liveInstanceCount initialization
//
//    LightVolume is malloc'd (not calloc'd).  liveInstanceCount is only set
//    inside updateLightUniformBuffers.  executeLightPass reads it BEFORE the
//    UBO fill.  On the first frame, garbage -> GPU hang.
// ---------------------------------------------------------------------------

void test_light_liveInstanceCount_zero_before_first_fill(void)
{
    LightManager man;
    memset(&man, 0, sizeof(man));
    man.lightLen = 1;
    man.pLight   = (LightVolume*) calloc(1, sizeof(LightVolume));

    man.pLight[0].lightVolumeInstanceCount = 4;
    man.pLight[0].liveInstanceCount        = 0; // must be 0 for safe first frame
    man.pLight[0].alive                    = LIGHT_ALIVE;
    man.pLight[0].pLightVolumeInstance     = (LightVolumeInstance*) calloc(4, sizeof(LightVolumeInstance));
    for (uint32_t i = 0; i < 4; i++)
        man.pLight[0].pLightVolumeInstance[i].alive = LIGHT_INSTANCE_ALIVE;

    man.lightInstanceCapacity       = 4;
    man.lightInstanceCount          = 0;
    man.instanceOffset.items        = (uint32_t*) calloc(1, sizeof(uint32_t));
    man.instanceOffset.itemsLen     = 1;

    // Before first fill, must be 0
    ASSERT_EQ(man.pLight[0].liveInstanceCount, 0);

    // After fill, should be updated
    SCENE scene;
    memset(&scene, 0, sizeof(scene));
    scene.lightManager = man;

    GraphicSwapchain sc  = make_fake_swapchain();
    Camera3D         cam = make_test_camera();
    LightTransformations* result = updateLightUniformBuffers(&sc, 0.0f, cam, &scene);

    ASSERT_EQ(scene.lightManager.pLight[0].liveInstanceCount, 4);

    free(result);
    free(man.pLight[0].pLightVolumeInstance);
    free(man.pLight);
    free(man.instanceOffset.items);
}

// ---------------------------------------------------------------------------
// 4. queryLightInstanceAlive on disabled (not destroyed) light
//
//    queryLightInstanceAlive checks !light->pLightVolumeInstance (not alive).
//    For disabled light (alive=2), pLightVolumeInstance is still valid,
//    so the query should return the actual instance state.
// ---------------------------------------------------------------------------

void test_query_light_instance_on_disabled_light(void)
{
    LightManager man = make_test_light_manager(1, 4);

    disableLight(&man, 0);
    ASSERT_EQ(man.pLight[0].alive, LIGHT_DISABLED);

    // pLightVolumeInstance is still valid — query should work
    ASSERT_EQ(queryLightInstanceAlive(&man, 0, 0), LIGHT_INSTANCE_ALIVE);
    ASSERT_EQ(queryLightInstanceAlive(&man, 0, 3), LIGHT_INSTANCE_ALIVE);

    // Destroy an instance while light is disabled
    destroyLightInstance(&man, 0, 1);
    ASSERT_EQ(queryLightInstanceAlive(&man, 0, 1), LIGHT_INSTANCE_DESTROYED);

    free_test_light_manager(&man);
}

// ---------------------------------------------------------------------------
// 5. UBO fill after destroyLight (pLightVolumeInstance freed)
//
//    destroyLight frees pLightVolumeInstance and sets count to 0.
//    The UBO fill must not dereference the freed pointer.
// ---------------------------------------------------------------------------

void test_light_ubo_fill_after_destroy_light(void)
{
    SCENE*           scene = make_light_scene(2, 3);
    GraphicSwapchain sc    = make_fake_swapchain();
    Camera3D         cam   = make_test_camera();

    destroyLight(&scene->lightManager, 0);

    // pLightVolumeInstance is NULL, lightVolumeInstanceCount is 0
    ASSERT_NULL(scene->lightManager.pLight[0].pLightVolumeInstance);
    ASSERT_EQ(scene->lightManager.pLight[0].lightVolumeInstanceCount, 0);

    // Must not crash — the alive != LIGHT_ALIVE check skips it
    LightTransformations* result = updateLightUniformBuffers(&sc, 0.0f, cam, scene);

    ASSERT_EQ(scene->lightManager.pLight[0].liveInstanceCount, 0);
    ASSERT_EQ(scene->lightManager.pLight[1].liveInstanceCount, 3);

    free(result);
    free_light_scene(scene);
}

// ---------------------------------------------------------------------------
// 6. Compacted offsets monotonicity (light side)
// ---------------------------------------------------------------------------

void test_light_compacted_offsets_monotonic(void)
{
    SCENE*           scene = make_light_scene(4, 3);
    GraphicSwapchain sc    = make_fake_swapchain();
    Camera3D         cam   = make_test_camera();

    destroyLightInstance(&scene->lightManager, 0, 1);
    disableLight(&scene->lightManager, 1);
    destroyLightInstance(&scene->lightManager, 2, 0);
    destroyLightInstance(&scene->lightManager, 2, 2);

    LightTransformations* result = updateLightUniformBuffers(&sc, 0.0f, cam, scene);

    // Offsets non-decreasing
    for (uint32_t i = 1; i < scene->lightManager.lightLen; i++)
    {
        ASSERT_EQ(scene->lightManager.instanceOffset.items[i] >=
                  scene->lightManager.instanceOffset.items[i - 1], 1);
    }

    // Each offset = prev offset + prev liveCount
    for (uint32_t i = 1; i < scene->lightManager.lightLen; i++)
    {
        uint32_t expected = scene->lightManager.instanceOffset.items[i - 1]
                          + scene->lightManager.pLight[i - 1].liveInstanceCount;
        ASSERT_EQ(scene->lightManager.instanceOffset.items[i], expected);
    }

    // Sum check
    uint32_t last = scene->lightManager.lightLen - 1;
    uint32_t total = scene->lightManager.instanceOffset.items[last]
                   + scene->lightManager.pLight[last].liveInstanceCount;
    ASSERT_EQ(total, scene->lightManager.lightInstanceCount);

    free(result);
    free_light_scene(scene);
}

// ---------------------------------------------------------------------------
// 7. All lights dead — triggers zero total (camera-only fallback analog)
// ---------------------------------------------------------------------------

void test_all_lights_dead_zero_total(void)
{
    SCENE*           scene = make_light_scene(2, 3);
    GraphicSwapchain sc    = make_fake_swapchain();
    Camera3D         cam   = make_test_camera();

    destroyLight(&scene->lightManager, 0);
    destroyLight(&scene->lightManager, 1);

    LightTransformations* result = updateLightUniformBuffers(&sc, 0.0f, cam, scene);

    ASSERT_EQ(scene->lightManager.lightInstanceCount, 0);
    for (uint32_t i = 0; i < scene->lightManager.lightLen; i++)
        ASSERT_EQ(scene->lightManager.pLight[i].liveInstanceCount, 0);

    free(result);
    free_light_scene(scene);
}

// ---------------------------------------------------------------------------
// 8. Double-disable / double-enable idempotency
// ---------------------------------------------------------------------------

void test_light_double_disable_instance_idempotent(void)
{
    LightManager man = make_test_light_manager(1, 4);

    disableLightInstance(&man, 0, 0);
    ASSERT_EQ(man.pLight[0].pLightVolumeInstance[0].alive, LIGHT_INSTANCE_DISABLED);

    // Second disable — already disabled, should be a no-op
    disableLightInstance(&man, 0, 0);
    ASSERT_EQ(man.pLight[0].pLightVolumeInstance[0].alive, LIGHT_INSTANCE_DISABLED);

    free_test_light_manager(&man);
}

void test_light_double_enable_instance_idempotent(void)
{
    LightManager man = make_test_light_manager(1, 4);

    // Already alive — enable should be a no-op
    enableLightInstance(&man, 0, 0);
    ASSERT_EQ(man.pLight[0].pLightVolumeInstance[0].alive, LIGHT_INSTANCE_ALIVE);

    free_test_light_manager(&man);
}

void test_model_double_disable_instance_idempotent(void)
{
    Models3DManager man = make_test_manager(1, 4);

    disableInstance(&man, 0, 0);
    ASSERT_EQ(man.pDenseModelInstances[0].alive, INSTANCE_DISABLED);

    // Second disable — already disabled, should be a no-op
    disableInstance(&man, 0, 0);
    ASSERT_EQ(man.pDenseModelInstances[0].alive, INSTANCE_DISABLED);

    free_test_manager(&man);
}

void test_model_double_enable_instance_idempotent(void)
{
    Models3DManager man = make_test_manager(1, 4);

    // Already alive — enable should be a no-op
    enableInstance(&man, 0, 0);
    ASSERT_EQ(man.pDenseModelInstances[0].alive, INSTANCE_ALIVE);

    free_test_manager(&man);
}

// ---------------------------------------------------------------------------
// 9. Single-light single-instance full lifecycle with UBO
// ---------------------------------------------------------------------------

void test_single_light_single_instance_lifecycle(void)
{
    SCENE*           scene = make_light_scene(1, 1);
    GraphicSwapchain sc    = make_fake_swapchain();
    Camera3D         cam   = make_test_camera();

    // Alive
    LightTransformations* r1 = updateLightUniformBuffers(&sc, 0.0f, cam, scene);
    ASSERT_EQ(scene->lightManager.pLight[0].liveInstanceCount, 1);
    ASSERT_EQ(scene->lightManager.lightInstanceCount, 1);
    free(r1);

    // Disable
    disableLightInstance(&scene->lightManager, 0, 0);
    LightTransformations* r2 = updateLightUniformBuffers(&sc, 0.016f, cam, scene);
    ASSERT_EQ(scene->lightManager.pLight[0].liveInstanceCount, 0);
    ASSERT_EQ(scene->lightManager.lightInstanceCount, 0);
    free(r2);

    // Re-enable
    enableLightInstance(&scene->lightManager, 0, 0);
    LightTransformations* r3 = updateLightUniformBuffers(&sc, 0.032f, cam, scene);
    ASSERT_EQ(scene->lightManager.pLight[0].liveInstanceCount, 1);
    free(r3);

    // Destroy
    destroyLightInstance(&scene->lightManager, 0, 0);
    LightTransformations* r4 = updateLightUniformBuffers(&sc, 0.048f, cam, scene);
    ASSERT_EQ(scene->lightManager.pLight[0].liveInstanceCount, 0);
    ASSERT_EQ(scene->lightManager.lightInstanceCount, 0);
    free(r4);

    free_light_scene(scene);
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main(void)
{
    printf("=== Light Frame Safety Tests ===\n");

    RUN_TEST(test_light_draw_params_within_bounds_all_alive);
    RUN_TEST(test_light_draw_params_within_bounds_mixed_state);
    RUN_TEST(test_light_memcpy_len_within_capacity);
    RUN_TEST(test_light_liveInstanceCount_zero_before_first_fill);
    RUN_TEST(test_query_light_instance_on_disabled_light);
    RUN_TEST(test_light_ubo_fill_after_destroy_light);
    RUN_TEST(test_light_compacted_offsets_monotonic);
    RUN_TEST(test_all_lights_dead_zero_total);
    RUN_TEST(test_light_double_disable_instance_idempotent);
    RUN_TEST(test_light_double_enable_instance_idempotent);
    RUN_TEST(test_model_double_disable_instance_idempotent);
    RUN_TEST(test_model_double_enable_instance_idempotent);
    RUN_TEST(test_single_light_single_instance_lifecycle);

    TEST_SUMMARY();
}
