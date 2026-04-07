#include "test_harness.h"
#include "test_helpers.h"
#include "uniformBuffer.h"
#include "light.h"

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

static SCENE* make_light_test_scene(uint32_t lightCount, uint32_t instancesPerLight)
{
    SCENE* scene = (SCENE*) calloc(1, sizeof(SCENE));

    scene->lightManager = make_test_light_manager(lightCount, instancesPerLight);

    scene->camera.eye[2] = 5.0f;
    scene->camera.up[1]  = 1.0f;

    return scene;
}

static void free_light_test_scene(SCENE* scene)
{
    if (!scene)
        return;
    free_test_light_manager(&scene->lightManager);
    free(scene);
}

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

void test_light_all_alive_full_output(void)
{
    SCENE*           scene = make_light_test_scene(2, 3);
    GraphicSwapchain sc    = make_fake_swapchain();
    Camera3D         cam   = make_test_camera();

    LightTransformations* result = updateLightUniformBuffers(&sc, 0.0f, cam, scene);

    ASSERT_EQ(scene->lightManager.lightInstanceCount, 6);
    ASSERT_EQ(scene->lightManager.pLight[0].liveInstanceCount, 3);
    ASSERT_EQ(scene->lightManager.pLight[1].liveInstanceCount, 3);
    ASSERT_EQ(scene->lightManager.instanceOffset.items[0], 0);
    ASSERT_EQ(scene->lightManager.instanceOffset.items[1], 3);

    free(result);
    free_light_test_scene(scene);
}

void test_light_one_instance_destroyed_compacted(void)
{
    SCENE*           scene = make_light_test_scene(1, 4);
    GraphicSwapchain sc    = make_fake_swapchain();
    Camera3D         cam   = make_test_camera();

    scene->lightManager.pLight[0].pLightVolumeInstance[1].alive = LIGHT_INSTANCE_DESTROYED;

    LightTransformations* result = updateLightUniformBuffers(&sc, 0.0f, cam, scene);

    ASSERT_EQ(scene->lightManager.pLight[0].liveInstanceCount, 3);
    ASSERT_EQ(scene->lightManager.lightInstanceCount, 3);

    free(result);
    free_light_test_scene(scene);
}

void test_light_one_instance_disabled_compacted(void)
{
    SCENE*           scene = make_light_test_scene(1, 4);
    GraphicSwapchain sc    = make_fake_swapchain();
    Camera3D         cam   = make_test_camera();

    scene->lightManager.pLight[0].pLightVolumeInstance[2].alive = LIGHT_INSTANCE_DISABLED;

    LightTransformations* result = updateLightUniformBuffers(&sc, 0.0f, cam, scene);

    ASSERT_EQ(scene->lightManager.pLight[0].liveInstanceCount, 3);
    ASSERT_EQ(scene->lightManager.lightInstanceCount, 3);

    free(result);
    free_light_test_scene(scene);
}

void test_light_all_instances_dead_zero_output(void)
{
    SCENE*           scene = make_light_test_scene(1, 4);
    GraphicSwapchain sc    = make_fake_swapchain();
    Camera3D         cam   = make_test_camera();

    for (uint32_t i = 0; i < 4; i++)
        scene->lightManager.pLight[0].pLightVolumeInstance[i].alive = LIGHT_INSTANCE_DESTROYED;

    LightTransformations* result = updateLightUniformBuffers(&sc, 0.0f, cam, scene);

    ASSERT_EQ(scene->lightManager.pLight[0].liveInstanceCount, 0);
    ASSERT_EQ(scene->lightManager.lightInstanceCount, 0);

    free(result);
    free_light_test_scene(scene);
}

void test_light_disabled_skips_all_instances(void)
{
    SCENE*           scene = make_light_test_scene(2, 3);
    GraphicSwapchain sc    = make_fake_swapchain();
    Camera3D         cam   = make_test_camera();

    scene->lightManager.pLight[0].alive = LIGHT_DISABLED;

    LightTransformations* result = updateLightUniformBuffers(&sc, 0.0f, cam, scene);

    ASSERT_EQ(scene->lightManager.pLight[0].liveInstanceCount, 0);
    ASSERT_EQ(scene->lightManager.pLight[1].liveInstanceCount, 3);
    ASSERT_EQ(scene->lightManager.lightInstanceCount, 3);
    // Model 1 offset should be 0 (compacted past disabled light 0)
    ASSERT_EQ(scene->lightManager.instanceOffset.items[1], 0);

    free(result);
    free_light_test_scene(scene);
}

void test_light_destroyed_skips_all_instances(void)
{
    SCENE*           scene = make_light_test_scene(1, 4);
    GraphicSwapchain sc    = make_fake_swapchain();
    Camera3D         cam   = make_test_camera();

    scene->lightManager.pLight[0].alive = LIGHT_DESTROYED;

    LightTransformations* result = updateLightUniformBuffers(&sc, 0.0f, cam, scene);

    ASSERT_EQ(scene->lightManager.pLight[0].liveInstanceCount, 0);
    ASSERT_EQ(scene->lightManager.lightInstanceCount, 0);

    free(result);
    free_light_test_scene(scene);
}

void test_light_mixed_state_multi_light(void)
{
    SCENE*           scene = make_light_test_scene(3, 4);
    GraphicSwapchain sc    = make_fake_swapchain();
    Camera3D         cam   = make_test_camera();

    // Light 0: kill instance 1 and 3 -> 2 live
    scene->lightManager.pLight[0].pLightVolumeInstance[1].alive = LIGHT_INSTANCE_DESTROYED;
    scene->lightManager.pLight[0].pLightVolumeInstance[3].alive = LIGHT_INSTANCE_DISABLED;
    // Light 1: disable entire light -> 0 live
    scene->lightManager.pLight[1].alive = LIGHT_DISABLED;
    // Light 2: all alive -> 4 live

    LightTransformations* result = updateLightUniformBuffers(&sc, 0.0f, cam, scene);

    ASSERT_EQ(scene->lightManager.pLight[0].liveInstanceCount, 2);
    ASSERT_EQ(scene->lightManager.pLight[1].liveInstanceCount, 0);
    ASSERT_EQ(scene->lightManager.pLight[2].liveInstanceCount, 4);
    ASSERT_EQ(scene->lightManager.lightInstanceCount, 6);
    ASSERT_EQ(scene->lightManager.instanceOffset.items[0], 0);
    ASSERT_EQ(scene->lightManager.instanceOffset.items[1], 2);
    ASSERT_EQ(scene->lightManager.instanceOffset.items[2], 2);

    free(result);
    free_light_test_scene(scene);
}

void test_light_ubo_multi_frame_stability(void)
{
    SCENE*           scene = make_light_test_scene(2, 3);
    GraphicSwapchain sc    = make_fake_swapchain();
    Camera3D         cam   = make_test_camera();

    // Frame 1
    LightTransformations* r1 = updateLightUniformBuffers(&sc, 0.0f, cam, scene);
    ASSERT_EQ(scene->lightManager.pLight[0].liveInstanceCount, 3);
    ASSERT_EQ(scene->lightManager.pLight[1].liveInstanceCount, 3);
    free(r1);

    // Kill an instance
    destroyLightInstance(&scene->lightManager, 0, 0);

    // Frame 2
    LightTransformations* r2 = updateLightUniformBuffers(&sc, 0.016f, cam, scene);
    ASSERT_EQ(scene->lightManager.pLight[0].liveInstanceCount, 2);
    ASSERT_EQ(scene->lightManager.pLight[1].liveInstanceCount, 3);
    ASSERT_EQ(scene->lightManager.lightInstanceCount, 5);
    free(r2);

    // Disable light 1
    disableLight(&scene->lightManager, 1);

    // Frame 3
    LightTransformations* r3 = updateLightUniformBuffers(&sc, 0.032f, cam, scene);
    ASSERT_EQ(scene->lightManager.pLight[0].liveInstanceCount, 2);
    ASSERT_EQ(scene->lightManager.pLight[1].liveInstanceCount, 0);
    ASSERT_EQ(scene->lightManager.lightInstanceCount, 2);
    free(r3);

    // Re-enable light 1
    enableLight(&scene->lightManager, 1);

    // Frame 4
    LightTransformations* r4 = updateLightUniformBuffers(&sc, 0.048f, cam, scene);
    ASSERT_EQ(scene->lightManager.pLight[0].liveInstanceCount, 2);
    ASSERT_EQ(scene->lightManager.pLight[1].liveInstanceCount, 3);
    ASSERT_EQ(scene->lightManager.lightInstanceCount, 5);
    free(r4);

    free_light_test_scene(scene);
}

void test_light_destroy_then_ubo_fill(void)
{
    SCENE*           scene = make_light_test_scene(2, 3);
    GraphicSwapchain sc    = make_fake_swapchain();
    Camera3D         cam   = make_test_camera();

    destroyLight(&scene->lightManager, 0);

    LightTransformations* result = updateLightUniformBuffers(&sc, 0.0f, cam, scene);

    ASSERT_EQ(scene->lightManager.pLight[0].liveInstanceCount, 0);
    ASSERT_EQ(scene->lightManager.pLight[1].liveInstanceCount, 3);
    ASSERT_EQ(scene->lightManager.lightInstanceCount, 3);
    ASSERT_EQ(scene->lightManager.instanceOffset.items[1], 0);

    free(result);
    free_light_test_scene(scene);
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main(void)
{
    printf("=== Light UBO Compaction Tests ===\n");

    RUN_TEST(test_light_all_alive_full_output);
    RUN_TEST(test_light_one_instance_destroyed_compacted);
    RUN_TEST(test_light_one_instance_disabled_compacted);
    RUN_TEST(test_light_all_instances_dead_zero_output);
    RUN_TEST(test_light_disabled_skips_all_instances);
    RUN_TEST(test_light_destroyed_skips_all_instances);
    RUN_TEST(test_light_mixed_state_multi_light);
    RUN_TEST(test_light_ubo_multi_frame_stability);
    RUN_TEST(test_light_destroy_then_ubo_fill);

    TEST_SUMMARY();
}
