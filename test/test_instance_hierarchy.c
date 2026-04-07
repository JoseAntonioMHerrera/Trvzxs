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
    cam.eye[2] = 5.0f;
    cam.up[1]  = 1.0f;
    return cam;
}

// ---------------------------------------------------------------------------
// 1. Parent-child: child world = parent.local + child.local
// ---------------------------------------------------------------------------

void test_parent_child_position(void)
{
    // 1 model, 2 instances: instance 0 = parent, instance 1 = child
    SCENE*           scene = make_test_scene(1, 2);
    GraphicSwapchain sc    = make_fake_swapchain();
    Camera3D         cam   = make_test_camera();

    Model3DInstance* parent = &scene->modelsManager.pDenseModelInstances[0];
    Model3DInstance* child  = &scene->modelsManager.pDenseModelInstances[1];

    instanceSetPosition(parent, (vec3){10.0f, 0.0f, 0.0f});
    instanceSetPosition(child,  (vec3){0.0f, 5.0f, 0.0f});

    instanceSetParent(&scene->modelsManager, 0, 1, 0, 0);

    MODEL_VIEW_PROJECTION* result = updateModelUniformBuffers(&sc, 0.0f, cam, scene);

    // Parent world = its own local: (10, 0, 0)
    ASSERT_FLOAT_EQ(parent->m[3][0], 10.0f);
    ASSERT_FLOAT_EQ(parent->m[3][1], 0.0f);
    ASSERT_FLOAT_EQ(parent->m[3][2], 0.0f);

    // Child world = parent + local: (10, 5, 0)
    ASSERT_FLOAT_EQ(child->m[3][0], 10.0f);
    ASSERT_FLOAT_EQ(child->m[3][1], 5.0f);
    ASSERT_FLOAT_EQ(child->m[3][2], 0.0f);

    free(result);
    free_test_scene(scene);
}

// ---------------------------------------------------------------------------
// 2. Three-level chain: grandparent -> parent -> child
// ---------------------------------------------------------------------------

void test_three_level_chain(void)
{
    // 1 model, 3 instances: 0=grandparent, 1=parent, 2=child
    SCENE*           scene = make_test_scene(1, 3);
    GraphicSwapchain sc    = make_fake_swapchain();
    Camera3D         cam   = make_test_camera();

    Model3DInstance* gp    = &scene->modelsManager.pDenseModelInstances[0];
    Model3DInstance* mid   = &scene->modelsManager.pDenseModelInstances[1];
    Model3DInstance* child = &scene->modelsManager.pDenseModelInstances[2];

    instanceSetPosition(gp,    (vec3){10.0f, 0.0f, 0.0f});
    instanceSetPosition(mid,   (vec3){0.0f, 20.0f, 0.0f});
    instanceSetPosition(child, (vec3){0.0f, 0.0f, 30.0f});

    instanceSetParent(&scene->modelsManager, 0, 1, 0, 0); // mid -> gp
    instanceSetParent(&scene->modelsManager, 0, 2, 0, 1); // child -> mid

    MODEL_VIEW_PROJECTION* result = updateModelUniformBuffers(&sc, 0.0f, cam, scene);

    // Grandparent: (10, 0, 0)
    ASSERT_FLOAT_EQ(gp->m[3][0], 10.0f);
    ASSERT_FLOAT_EQ(gp->m[3][1], 0.0f);
    ASSERT_FLOAT_EQ(gp->m[3][2], 0.0f);

    // Parent: gp + local = (10, 20, 0)
    ASSERT_FLOAT_EQ(mid->m[3][0], 10.0f);
    ASSERT_FLOAT_EQ(mid->m[3][1], 20.0f);
    ASSERT_FLOAT_EQ(mid->m[3][2], 0.0f);

    // Child: (gp + mid.local) + child.local = (10, 20, 30)
    ASSERT_FLOAT_EQ(child->m[3][0], 10.0f);
    ASSERT_FLOAT_EQ(child->m[3][1], 20.0f);
    ASSERT_FLOAT_EQ(child->m[3][2], 30.0f);

    free(result);
    free_test_scene(scene);
}

// ---------------------------------------------------------------------------
// 3. Cross-model parenting: child in model 1 parented to instance in model 0
// ---------------------------------------------------------------------------

void test_cross_model_parenting(void)
{
    // 2 models, 2 instances each. Global: model0=[0,1], model1=[2,3]
    SCENE*           scene = make_test_scene(2, 2);
    GraphicSwapchain sc    = make_fake_swapchain();
    Camera3D         cam   = make_test_camera();

    // Parent: model 0, instance 1 (global 1)
    Model3DInstance* parent = &scene->modelsManager.pDenseModelInstances[1];
    // Child: model 1, instance 0 (global 2)
    Model3DInstance* child  = &scene->modelsManager.pDenseModelInstances[2];

    instanceSetPosition(parent, (vec3){5.0f, 5.0f, 5.0f});
    instanceSetPosition(child,  (vec3){1.0f, 1.0f, 1.0f});

    instanceSetParent(&scene->modelsManager, 1, 0, 0, 1);

    MODEL_VIEW_PROJECTION* result = updateModelUniformBuffers(&sc, 0.0f, cam, scene);

    // Child world = parent(5,5,5) + local(1,1,1) = (6,6,6)
    ASSERT_FLOAT_EQ(child->m[3][0], 6.0f);
    ASSERT_FLOAT_EQ(child->m[3][1], 6.0f);
    ASSERT_FLOAT_EQ(child->m[3][2], 6.0f);

    free(result);
    free_test_scene(scene);
}

// ---------------------------------------------------------------------------
// 4. Parent with uniform scale affects child world position
// ---------------------------------------------------------------------------

void test_parent_scale_affects_child(void)
{
    SCENE*           scene = make_test_scene(1, 2);
    GraphicSwapchain sc    = make_fake_swapchain();
    Camera3D         cam   = make_test_camera();

    Model3DInstance* parent = &scene->modelsManager.pDenseModelInstances[0];
    Model3DInstance* child  = &scene->modelsManager.pDenseModelInstances[1];

    instanceSetPosition(parent, (vec3){0.0f, 0.0f, 0.0f});
    instanceSetScaleUniform(parent, 2.0f);

    instanceSetPosition(child, (vec3){5.0f, 0.0f, 0.0f});

    instanceSetParent(&scene->modelsManager, 0, 1, 0, 0);

    MODEL_VIEW_PROJECTION* result = updateModelUniformBuffers(&sc, 0.0f, cam, scene);

    // Parent at origin, scale 2x. Child local (5,0,0) -> world (10,0,0)
    ASSERT_FLOAT_EQ(child->m[3][0], 10.0f);
    ASSERT_FLOAT_EQ(child->m[3][1], 0.0f);
    ASSERT_FLOAT_EQ(child->m[3][2], 0.0f);

    free(result);
    free_test_scene(scene);
}

// ---------------------------------------------------------------------------
// 5. Multiple children under same parent
// ---------------------------------------------------------------------------

void test_multiple_children_same_parent(void)
{
    SCENE*           scene = make_test_scene(1, 4);
    GraphicSwapchain sc    = make_fake_swapchain();
    Camera3D         cam   = make_test_camera();

    instanceSetPosition(&scene->modelsManager.pDenseModelInstances[0], (vec3){10.0f, 0.0f, 0.0f});
    instanceSetPosition(&scene->modelsManager.pDenseModelInstances[1], (vec3){1.0f, 0.0f, 0.0f});
    instanceSetPosition(&scene->modelsManager.pDenseModelInstances[2], (vec3){2.0f, 0.0f, 0.0f});
    instanceSetPosition(&scene->modelsManager.pDenseModelInstances[3], (vec3){3.0f, 0.0f, 0.0f});

    instanceSetParent(&scene->modelsManager, 0, 1, 0, 0);
    instanceSetParent(&scene->modelsManager, 0, 2, 0, 0);
    instanceSetParent(&scene->modelsManager, 0, 3, 0, 0);

    MODEL_VIEW_PROJECTION* result = updateModelUniformBuffers(&sc, 0.0f, cam, scene);

    ASSERT_FLOAT_EQ(scene->modelsManager.pDenseModelInstances[0].m[3][0], 10.0f);
    ASSERT_FLOAT_EQ(scene->modelsManager.pDenseModelInstances[1].m[3][0], 11.0f);
    ASSERT_FLOAT_EQ(scene->modelsManager.pDenseModelInstances[2].m[3][0], 12.0f);
    ASSERT_FLOAT_EQ(scene->modelsManager.pDenseModelInstances[3].m[3][0], 13.0f);

    free(result);
    free_test_scene(scene);
}

// ---------------------------------------------------------------------------
// 6. Cascade-disable: disabled parent -> child and grandchild skipped
// ---------------------------------------------------------------------------

void test_cascade_disable_skips_chain(void)
{
    SCENE*           scene = make_test_scene(1, 3);
    GraphicSwapchain sc    = make_fake_swapchain();
    Camera3D         cam   = make_test_camera();

    instanceSetPosition(&scene->modelsManager.pDenseModelInstances[0], (vec3){10.0f, 0.0f, 0.0f});
    instanceSetPosition(&scene->modelsManager.pDenseModelInstances[1], (vec3){0.0f, 20.0f, 0.0f});
    instanceSetPosition(&scene->modelsManager.pDenseModelInstances[2], (vec3){0.0f, 0.0f, 30.0f});

    instanceSetParent(&scene->modelsManager, 0, 1, 0, 0);
    instanceSetParent(&scene->modelsManager, 0, 2, 0, 1);

    // Disable grandparent
    scene->modelsManager.pDenseModelInstances[0].alive = INSTANCE_DISABLED;

    MODEL_VIEW_PROJECTION* result = updateModelUniformBuffers(&sc, 0.0f, cam, scene);

    // All 3 skipped
    ASSERT_EQ(scene->modelsManager.pModel[0].liveInstanceCount, 0);
    ASSERT_EQ(scene->modelsManager.modelInstanceLen, 0);

    // Children should have PASS_DISABLED set
    ASSERT_EQ(scene->modelsManager.pDenseModelInstances[1].passFlags, PASS_DISABLED);
    ASSERT_EQ(scene->modelsManager.pDenseModelInstances[2].passFlags, PASS_DISABLED);

    free(result);
    free_test_scene(scene);
}

// ---------------------------------------------------------------------------
// 7. Re-enable parent -> children reappear next frame with correct position
// ---------------------------------------------------------------------------

void test_reenable_parent_restores_children(void)
{
    SCENE*           scene = make_test_scene(1, 2);
    GraphicSwapchain sc    = make_fake_swapchain();
    Camera3D         cam   = make_test_camera();

    instanceSetPosition(&scene->modelsManager.pDenseModelInstances[0], (vec3){10.0f, 0.0f, 0.0f});
    instanceSetPosition(&scene->modelsManager.pDenseModelInstances[1], (vec3){0.0f, 5.0f, 0.0f});

    instanceSetParent(&scene->modelsManager, 0, 1, 0, 0);

    // Frame 1: disable parent -> both skipped
    scene->modelsManager.pDenseModelInstances[0].alive = INSTANCE_DISABLED;
    MODEL_VIEW_PROJECTION* r1 = updateModelUniformBuffers(&sc, 0.0f, cam, scene);
    ASSERT_EQ(scene->modelsManager.pModel[0].liveInstanceCount, 0);
    free(r1);

    // Frame 2: re-enable parent -> both reappear
    scene->modelsManager.pDenseModelInstances[0].alive = INSTANCE_ALIVE;
    MODEL_VIEW_PROJECTION* r2 = updateModelUniformBuffers(&sc, 0.016f, cam, scene);
    ASSERT_EQ(scene->modelsManager.pModel[0].liveInstanceCount, 2);

    // Child world position correct: (10, 5, 0)
    ASSERT_FLOAT_EQ(scene->modelsManager.pDenseModelInstances[1].m[3][0], 10.0f);
    ASSERT_FLOAT_EQ(scene->modelsManager.pDenseModelInstances[1].m[3][1], 5.0f);
    ASSERT_FLOAT_EQ(scene->modelsManager.pDenseModelInstances[1].m[3][2], 0.0f);

    free(r2);
    free_test_scene(scene);
}

// ---------------------------------------------------------------------------
// 8. Clear parent -> child becomes root, uses only local transform
// ---------------------------------------------------------------------------

void test_clear_parent_makes_root(void)
{
    SCENE*           scene = make_test_scene(1, 2);
    GraphicSwapchain sc    = make_fake_swapchain();
    Camera3D         cam   = make_test_camera();

    instanceSetPosition(&scene->modelsManager.pDenseModelInstances[0], (vec3){10.0f, 0.0f, 0.0f});
    instanceSetPosition(&scene->modelsManager.pDenseModelInstances[1], (vec3){0.0f, 5.0f, 0.0f});

    instanceSetParent(&scene->modelsManager, 0, 1, 0, 0);

    // Frame 1: parented -> child at (10, 5, 0)
    MODEL_VIEW_PROJECTION* r1 = updateModelUniformBuffers(&sc, 0.0f, cam, scene);
    ASSERT_FLOAT_EQ(scene->modelsManager.pDenseModelInstances[1].m[3][0], 10.0f);
    ASSERT_FLOAT_EQ(scene->modelsManager.pDenseModelInstances[1].m[3][1], 5.0f);
    free(r1);

    // Clear parent
    instanceClearParent(&scene->modelsManager, 0, 1);

    // Frame 2: unparented -> child at (0, 5, 0)
    MODEL_VIEW_PROJECTION* r2 = updateModelUniformBuffers(&sc, 0.016f, cam, scene);
    ASSERT_FLOAT_EQ(scene->modelsManager.pDenseModelInstances[1].m[3][0], 0.0f);
    ASSERT_FLOAT_EQ(scene->modelsManager.pDenseModelInstances[1].m[3][1], 5.0f);
    ASSERT_FLOAT_EQ(scene->modelsManager.pDenseModelInstances[1].m[3][2], 0.0f);

    free(r2);
    free_test_scene(scene);
}

// ---------------------------------------------------------------------------
// 9. Ordering validation: parent after child rejected
// ---------------------------------------------------------------------------

void test_set_parent_rejects_wrong_order(void)
{
    Models3DManager man = make_test_manager(1, 2);

    // Try to parent instance 0 to instance 1 (wrong: parent must have lower index)
    CAPTURE_STDERR_BEGIN();
    instanceSetParent(&man, 0, 0, 0, 1);
    CAPTURE_STDERR_END();

    ASSERT_STDERR_CONTAINS("[instanceSetParent] rejected");
    ASSERT_EQ(man.pDenseModelInstances[0].parentGlobalIdx, INSTANCE_NO_PARENT);

    free_test_manager(&man);
}

// ---------------------------------------------------------------------------
// 10. Self-parenting rejected
// ---------------------------------------------------------------------------

void test_set_parent_rejects_self(void)
{
    Models3DManager man = make_test_manager(1, 2);

    CAPTURE_STDERR_BEGIN();
    instanceSetParent(&man, 0, 1, 0, 1);
    CAPTURE_STDERR_END();

    ASSERT_STDERR_CONTAINS("[instanceSetParent] rejected");
    ASSERT_EQ(man.pDenseModelInstances[1].parentGlobalIdx, INSTANCE_NO_PARENT);

    free_test_manager(&man);
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main(void)
{
    printf("=== Instance Hierarchy Tests ===\n");

    RUN_TEST(test_parent_child_position);
    RUN_TEST(test_three_level_chain);
    RUN_TEST(test_cross_model_parenting);
    RUN_TEST(test_parent_scale_affects_child);
    RUN_TEST(test_multiple_children_same_parent);
    RUN_TEST(test_cascade_disable_skips_chain);
    RUN_TEST(test_reenable_parent_restores_children);
    RUN_TEST(test_clear_parent_makes_root);
    RUN_TEST(test_set_parent_rejects_wrong_order);
    RUN_TEST(test_set_parent_rejects_self);

    TEST_SUMMARY();
}
