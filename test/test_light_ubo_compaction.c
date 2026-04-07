#include "test_harness.h"
#include "test_helpers.h"
#include "uniformBuffer.h"
#include "transformNode.h"
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

    // Allocate transforms for light instances
    uint32_t total = lightCount * instancesPerLight;
    scene->transformManager = make_test_transform_manager(total);

    // Wire transformIdx
    uint32_t tIdx = 0;
    for (uint32_t l = 0; l < lightCount; l++)
    {
        LightVolume* light = &scene->lightManager.pLight[l];
        for (uint32_t i = 0; i < instancesPerLight; i++)
        {
            scene->lightManager.pDenseLightInstances[light->denseBaseOffset + i].transformIdx = tIdx;
            tIdx++;
        }
    }

    scene->camera.eye[2] = 5.0f;
    scene->camera.up[1]  = 1.0f;

    return scene;
}

static void free_light_test_scene(SCENE* scene)
{
    if (!scene)
        return;
    free_test_light_manager(&scene->lightManager);
    free_test_transform_manager(&scene->transformManager);
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

    resolveTransforms(&scene->transformManager);
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

    scene->lightManager.pDenseLightInstances[scene->lightManager.pLight[0].denseBaseOffset +1].lifecycle = LIGHT_INSTANCE_DESTROYED;

    resolveTransforms(&scene->transformManager);
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

    scene->lightManager.pDenseLightInstances[scene->lightManager.pLight[0].denseBaseOffset +2].lifecycle = LIGHT_INSTANCE_DISABLED;

    resolveTransforms(&scene->transformManager);
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
        scene->lightManager.pDenseLightInstances[scene->lightManager.pLight[0].denseBaseOffset +i].lifecycle = LIGHT_INSTANCE_DESTROYED;

    resolveTransforms(&scene->transformManager);
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

    resolveTransforms(&scene->transformManager);
    LightTransformations* result = updateLightUniformBuffers(&sc, 0.0f, cam, scene);

    ASSERT_EQ(scene->lightManager.pLight[0].liveInstanceCount, 0);
    ASSERT_EQ(scene->lightManager.pLight[1].liveInstanceCount, 3);
    ASSERT_EQ(scene->lightManager.lightInstanceCount, 3);
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

    resolveTransforms(&scene->transformManager);
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

    scene->lightManager.pDenseLightInstances[scene->lightManager.pLight[0].denseBaseOffset +1].lifecycle = LIGHT_INSTANCE_DESTROYED;
    scene->lightManager.pDenseLightInstances[scene->lightManager.pLight[0].denseBaseOffset +3].lifecycle = LIGHT_INSTANCE_DISABLED;
    scene->lightManager.pLight[1].alive = LIGHT_DISABLED;

    resolveTransforms(&scene->transformManager);
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

    resolveTransforms(&scene->transformManager);
    LightTransformations* r1 = updateLightUniformBuffers(&sc, 0.0f, cam, scene);
    ASSERT_EQ(scene->lightManager.pLight[0].liveInstanceCount, 3);
    ASSERT_EQ(scene->lightManager.pLight[1].liveInstanceCount, 3);
    free(r1);

    destroyLightInstance(&scene->lightManager, 0, 0);

    resolveTransforms(&scene->transformManager);
    LightTransformations* r2 = updateLightUniformBuffers(&sc, 0.016f, cam, scene);
    ASSERT_EQ(scene->lightManager.pLight[0].liveInstanceCount, 2);
    ASSERT_EQ(scene->lightManager.pLight[1].liveInstanceCount, 3);
    ASSERT_EQ(scene->lightManager.lightInstanceCount, 5);
    free(r2);

    disableLight(&scene->lightManager, 1);

    resolveTransforms(&scene->transformManager);
    LightTransformations* r3 = updateLightUniformBuffers(&sc, 0.032f, cam, scene);
    ASSERT_EQ(scene->lightManager.pLight[0].liveInstanceCount, 2);
    ASSERT_EQ(scene->lightManager.pLight[1].liveInstanceCount, 0);
    ASSERT_EQ(scene->lightManager.lightInstanceCount, 2);
    free(r3);

    enableLight(&scene->lightManager, 1);

    resolveTransforms(&scene->transformManager);
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

    resolveTransforms(&scene->transformManager);
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
