#include "test_harness.h"
#include "test_helpers.h"
#include "uniformBuffer.h"
#include "transform.h"
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

// ---------------------------------------------------------------------------
// 1. Parent-child: child world = parent.local + child.local
// ---------------------------------------------------------------------------

void test_parent_child_position(void)
{
    SCENE*           scene = make_test_scene(1, 2);
    GraphicSwapchain sc    = make_fake_swapchain();
    Camera3D         cam   = make_test_camera();
    TransformManager* tman = &scene->transformManager;

    Model3DInstance* parent = &scene->modelsManager.pDenseModelInstances[0];
    Model3DInstance* child  = &scene->modelsManager.pDenseModelInstances[1];

    transformSetPosition(&tman->pTransformsCold[parent->transformIdx], (vec3){10.0f, 0.0f, 0.0f});
    transformSetPosition(&tman->pTransformsCold[child->transformIdx],  (vec3){0.0f, 5.0f, 0.0f});

    transformSetParent(tman, child->transformIdx, parent->transformIdx);

    resolveTransforms(tman);
    ModelInstanceUBO* result = (ModelInstanceUBO*) malloc(sizeof(ModelInstanceUBO) * scene->modelsManager.modelInstanceCapacity);
    updateModelUniformBuffersDirect(result, scene);

    // Parent world = its own local: (10, 0, 0)
    TransformHot* pNode = &tman->pTransformsHot[parent->transformIdx];
    ASSERT_FLOAT_EQ(pNode->worldMatrix[3][0], 10.0f);
    ASSERT_FLOAT_EQ(pNode->worldMatrix[3][1], 0.0f);
    ASSERT_FLOAT_EQ(pNode->worldMatrix[3][2], 0.0f);

    // Child world = parent + local: (10, 5, 0)
    TransformHot* cNode = &tman->pTransformsHot[child->transformIdx];
    ASSERT_FLOAT_EQ(cNode->worldMatrix[3][0], 10.0f);
    ASSERT_FLOAT_EQ(cNode->worldMatrix[3][1], 5.0f);
    ASSERT_FLOAT_EQ(cNode->worldMatrix[3][2], 0.0f);

    free(result);
    free_test_scene(scene);
}

// ---------------------------------------------------------------------------
// 2. Three-level chain: grandparent -> parent -> child
// ---------------------------------------------------------------------------

void test_three_level_chain(void)
{
    SCENE*           scene = make_test_scene(1, 3);
    GraphicSwapchain sc    = make_fake_swapchain();
    Camera3D         cam   = make_test_camera();
    TransformManager* tman = &scene->transformManager;

    Model3DInstance* gp    = &scene->modelsManager.pDenseModelInstances[0];
    Model3DInstance* mid   = &scene->modelsManager.pDenseModelInstances[1];
    Model3DInstance* child = &scene->modelsManager.pDenseModelInstances[2];

    transformSetPosition(&tman->pTransformsCold[gp->transformIdx],    (vec3){10.0f, 0.0f, 0.0f});
    transformSetPosition(&tman->pTransformsCold[mid->transformIdx],   (vec3){0.0f, 20.0f, 0.0f});
    transformSetPosition(&tman->pTransformsCold[child->transformIdx], (vec3){0.0f, 0.0f, 30.0f});

    transformSetParent(tman, mid->transformIdx, gp->transformIdx);
    transformSetParent(tman, child->transformIdx, mid->transformIdx);

    resolveTransforms(tman);
    ModelInstanceUBO* result = (ModelInstanceUBO*) malloc(sizeof(ModelInstanceUBO) * scene->modelsManager.modelInstanceCapacity);
    updateModelUniformBuffersDirect(result, scene);

    // Grandparent: (10, 0, 0)
    ASSERT_FLOAT_EQ(tman->pTransformsHot[gp->transformIdx].worldMatrix[3][0], 10.0f);
    ASSERT_FLOAT_EQ(tman->pTransformsHot[gp->transformIdx].worldMatrix[3][1], 0.0f);

    // Parent: gp + local = (10, 20, 0)
    ASSERT_FLOAT_EQ(tman->pTransformsHot[mid->transformIdx].worldMatrix[3][0], 10.0f);
    ASSERT_FLOAT_EQ(tman->pTransformsHot[mid->transformIdx].worldMatrix[3][1], 20.0f);

    // Child: (10, 20, 30)
    ASSERT_FLOAT_EQ(tman->pTransformsHot[child->transformIdx].worldMatrix[3][0], 10.0f);
    ASSERT_FLOAT_EQ(tman->pTransformsHot[child->transformIdx].worldMatrix[3][1], 20.0f);
    ASSERT_FLOAT_EQ(tman->pTransformsHot[child->transformIdx].worldMatrix[3][2], 30.0f);

    free(result);
    free_test_scene(scene);
}

// ---------------------------------------------------------------------------
// 3. Cross-model parenting: child in model 1 parented to instance in model 0
// ---------------------------------------------------------------------------

void test_cross_model_parenting(void)
{
    SCENE*           scene = make_test_scene(2, 2);
    GraphicSwapchain sc    = make_fake_swapchain();
    Camera3D         cam   = make_test_camera();
    TransformManager* tman = &scene->transformManager;

    Model3DInstance* parent = &scene->modelsManager.pDenseModelInstances[1];
    Model3DInstance* child  = &scene->modelsManager.pDenseModelInstances[2];

    transformSetPosition(&tman->pTransformsCold[parent->transformIdx], (vec3){5.0f, 5.0f, 5.0f});
    transformSetPosition(&tman->pTransformsCold[child->transformIdx],  (vec3){1.0f, 1.0f, 1.0f});

    transformSetParent(tman, child->transformIdx, parent->transformIdx);

    resolveTransforms(tman);
    ModelInstanceUBO* result = (ModelInstanceUBO*) malloc(sizeof(ModelInstanceUBO) * scene->modelsManager.modelInstanceCapacity);
    updateModelUniformBuffersDirect(result, scene);

    // Child world = parent(5,5,5) + local(1,1,1) = (6,6,6)
    TransformHot* cNode = &tman->pTransformsHot[child->transformIdx];
    ASSERT_FLOAT_EQ(cNode->worldMatrix[3][0], 6.0f);
    ASSERT_FLOAT_EQ(cNode->worldMatrix[3][1], 6.0f);
    ASSERT_FLOAT_EQ(cNode->worldMatrix[3][2], 6.0f);

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
    TransformManager* tman = &scene->transformManager;

    Model3DInstance* parent = &scene->modelsManager.pDenseModelInstances[0];
    Model3DInstance* child  = &scene->modelsManager.pDenseModelInstances[1];

    transformSetPosition(&tman->pTransformsCold[parent->transformIdx], (vec3){0.0f, 0.0f, 0.0f});
    transformSetScaleUniform(&tman->pTransformsCold[parent->transformIdx], 2.0f);

    transformSetPosition(&tman->pTransformsCold[child->transformIdx], (vec3){5.0f, 0.0f, 0.0f});

    transformSetParent(tman, child->transformIdx, parent->transformIdx);

    resolveTransforms(tman);
    ModelInstanceUBO* result = (ModelInstanceUBO*) malloc(sizeof(ModelInstanceUBO) * scene->modelsManager.modelInstanceCapacity);
    updateModelUniformBuffersDirect(result, scene);

    // Parent at origin, scale 2x. Child local (5,0,0) -> world (10,0,0)
    TransformHot* cNode = &tman->pTransformsHot[child->transformIdx];
    ASSERT_FLOAT_EQ(cNode->worldMatrix[3][0], 10.0f);
    ASSERT_FLOAT_EQ(cNode->worldMatrix[3][1], 0.0f);
    ASSERT_FLOAT_EQ(cNode->worldMatrix[3][2], 0.0f);

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
    TransformManager* tman = &scene->transformManager;

    transformSetPosition(&tman->pTransformsCold[scene->modelsManager.pDenseModelInstances[0].transformIdx], (vec3){10.0f, 0.0f, 0.0f});
    transformSetPosition(&tman->pTransformsCold[scene->modelsManager.pDenseModelInstances[1].transformIdx], (vec3){1.0f, 0.0f, 0.0f});
    transformSetPosition(&tman->pTransformsCold[scene->modelsManager.pDenseModelInstances[2].transformIdx], (vec3){2.0f, 0.0f, 0.0f});
    transformSetPosition(&tman->pTransformsCold[scene->modelsManager.pDenseModelInstances[3].transformIdx], (vec3){3.0f, 0.0f, 0.0f});

    transformSetParent(tman, scene->modelsManager.pDenseModelInstances[1].transformIdx,
                       scene->modelsManager.pDenseModelInstances[0].transformIdx);
    transformSetParent(tman, scene->modelsManager.pDenseModelInstances[2].transformIdx,
                       scene->modelsManager.pDenseModelInstances[0].transformIdx);
    transformSetParent(tman, scene->modelsManager.pDenseModelInstances[3].transformIdx,
                       scene->modelsManager.pDenseModelInstances[0].transformIdx);

    resolveTransforms(tman);
    ModelInstanceUBO* result = (ModelInstanceUBO*) malloc(sizeof(ModelInstanceUBO) * scene->modelsManager.modelInstanceCapacity);
    updateModelUniformBuffersDirect(result, scene);

    ASSERT_FLOAT_EQ(tman->pTransformsHot[scene->modelsManager.pDenseModelInstances[0].transformIdx].worldMatrix[3][0], 10.0f);
    ASSERT_FLOAT_EQ(tman->pTransformsHot[scene->modelsManager.pDenseModelInstances[1].transformIdx].worldMatrix[3][0], 11.0f);
    ASSERT_FLOAT_EQ(tman->pTransformsHot[scene->modelsManager.pDenseModelInstances[2].transformIdx].worldMatrix[3][0], 12.0f);
    ASSERT_FLOAT_EQ(tman->pTransformsHot[scene->modelsManager.pDenseModelInstances[3].transformIdx].worldMatrix[3][0], 13.0f);

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
    TransformManager* tman = &scene->transformManager;

    transformSetPosition(&tman->pTransformsCold[scene->modelsManager.pDenseModelInstances[0].transformIdx], (vec3){10.0f, 0.0f, 0.0f});
    transformSetPosition(&tman->pTransformsCold[scene->modelsManager.pDenseModelInstances[1].transformIdx], (vec3){0.0f, 20.0f, 0.0f});
    transformSetPosition(&tman->pTransformsCold[scene->modelsManager.pDenseModelInstances[2].transformIdx], (vec3){0.0f, 0.0f, 30.0f});

    transformSetParent(tman, scene->modelsManager.pDenseModelInstances[1].transformIdx,
                       scene->modelsManager.pDenseModelInstances[0].transformIdx);
    transformSetParent(tman, scene->modelsManager.pDenseModelInstances[2].transformIdx,
                       scene->modelsManager.pDenseModelInstances[1].transformIdx);

    // Disable grandparent — via transform lifecycle
    tman->pTransformsHot[scene->modelsManager.pDenseModelInstances[0].transformIdx].lifecycle = TRANSFORM_DISABLED;
    scene->modelsManager.pDenseModelInstances[0].lifecycle = INSTANCE_DISABLED;

    resolveTransforms(tman);
    ModelInstanceUBO* result = (ModelInstanceUBO*) malloc(sizeof(ModelInstanceUBO) * scene->modelsManager.modelInstanceCapacity);
    updateModelUniformBuffersDirect(result, scene);

    // All 3 skipped
    ASSERT_EQ(scene->modelsManager.pModel[0].liveInstanceCount, 0);
    ASSERT_EQ(scene->modelsManager.modelInstanceLen, 0);

    // Children transforms should have PASS_DISABLED set (DIRTY sticks on disabled nodes)
    ASSERT_EQ(tman->pTransformsHot[scene->modelsManager.pDenseModelInstances[1].transformIdx].passFlags & TRANSFORM_PASS_DISABLED, TRANSFORM_PASS_DISABLED);
    ASSERT_EQ(tman->pTransformsHot[scene->modelsManager.pDenseModelInstances[2].transformIdx].passFlags & TRANSFORM_PASS_DISABLED, TRANSFORM_PASS_DISABLED);

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
    TransformManager* tman = &scene->transformManager;

    transformSetPosition(&tman->pTransformsCold[scene->modelsManager.pDenseModelInstances[0].transformIdx], (vec3){10.0f, 0.0f, 0.0f});
    transformSetPosition(&tman->pTransformsCold[scene->modelsManager.pDenseModelInstances[1].transformIdx], (vec3){0.0f, 5.0f, 0.0f});

    transformSetParent(tman, scene->modelsManager.pDenseModelInstances[1].transformIdx,
                       scene->modelsManager.pDenseModelInstances[0].transformIdx);

    // Frame 1: disable parent -> both skipped
    tman->pTransformsHot[scene->modelsManager.pDenseModelInstances[0].transformIdx].lifecycle = TRANSFORM_DISABLED;
    scene->modelsManager.pDenseModelInstances[0].lifecycle = INSTANCE_DISABLED;

    resolveTransforms(tman);
    ModelInstanceUBO* r1 = (ModelInstanceUBO*) malloc(sizeof(ModelInstanceUBO) * scene->modelsManager.modelInstanceCapacity);
    updateModelUniformBuffersDirect(r1, scene);
    ASSERT_EQ(scene->modelsManager.pModel[0].liveInstanceCount, 0);
    free(r1);

    // Frame 2: re-enable parent -> both reappear
    tman->pTransformsHot[scene->modelsManager.pDenseModelInstances[0].transformIdx].lifecycle = TRANSFORM_ALIVE;
    scene->modelsManager.pDenseModelInstances[0].lifecycle = INSTANCE_ALIVE;

    resolveTransforms(tman);
    ModelInstanceUBO* r2 = (ModelInstanceUBO*) malloc(sizeof(ModelInstanceUBO) * scene->modelsManager.modelInstanceCapacity);
    updateModelUniformBuffersDirect(r2, scene);
    ASSERT_EQ(scene->modelsManager.pModel[0].liveInstanceCount, 2);

    // Child world position correct: (10, 5, 0)
    TransformHot* cNode = &tman->pTransformsHot[scene->modelsManager.pDenseModelInstances[1].transformIdx];
    ASSERT_FLOAT_EQ(cNode->worldMatrix[3][0], 10.0f);
    ASSERT_FLOAT_EQ(cNode->worldMatrix[3][1], 5.0f);
    ASSERT_FLOAT_EQ(cNode->worldMatrix[3][2], 0.0f);

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
    TransformManager* tman = &scene->transformManager;

    transformSetPosition(&tman->pTransformsCold[scene->modelsManager.pDenseModelInstances[0].transformIdx], (vec3){10.0f, 0.0f, 0.0f});
    transformSetPosition(&tman->pTransformsCold[scene->modelsManager.pDenseModelInstances[1].transformIdx], (vec3){0.0f, 5.0f, 0.0f});

    transformSetParent(tman, scene->modelsManager.pDenseModelInstances[1].transformIdx,
                       scene->modelsManager.pDenseModelInstances[0].transformIdx);

    // Frame 1: parented -> child at (10, 5, 0)
    resolveTransforms(tman);
    ModelInstanceUBO* r1 = (ModelInstanceUBO*) malloc(sizeof(ModelInstanceUBO) * scene->modelsManager.modelInstanceCapacity);
    updateModelUniformBuffersDirect(r1, scene);
    TransformHot* cNode = &tman->pTransformsHot[scene->modelsManager.pDenseModelInstances[1].transformIdx];
    ASSERT_FLOAT_EQ(cNode->worldMatrix[3][0], 10.0f);
    ASSERT_FLOAT_EQ(cNode->worldMatrix[3][1], 5.0f);
    free(r1);

    // Clear parent
    transformClearParent(tman, scene->modelsManager.pDenseModelInstances[1].transformIdx);

    // Frame 2: unparented -> child at (0, 5, 0)
    resolveTransforms(tman);
    ModelInstanceUBO* r2 = (ModelInstanceUBO*) malloc(sizeof(ModelInstanceUBO) * scene->modelsManager.modelInstanceCapacity);
    updateModelUniformBuffersDirect(r2, scene);
    ASSERT_FLOAT_EQ(cNode->worldMatrix[3][0], 0.0f);
    ASSERT_FLOAT_EQ(cNode->worldMatrix[3][1], 5.0f);
    ASSERT_FLOAT_EQ(cNode->worldMatrix[3][2], 0.0f);

    free(r2);
    free_test_scene(scene);
}

// ---------------------------------------------------------------------------
// 9. Ordering validation: parent after child rejected
// ---------------------------------------------------------------------------

void test_set_parent_rejects_wrong_order(void)
{
    Models3DManager  man  = make_test_manager(1, 2);
    TransformManager tman = make_test_transform_manager(2);

    man.pDenseModelInstances[0].transformIdx = 0;
    man.pDenseModelInstances[1].transformIdx = 1;

    // Try to parent instance 0 to instance 1 (wrong: parent must have lower index)
    CAPTURE_STDERR_BEGIN();
    transformSetParent(&tman, 0, 1);
    CAPTURE_STDERR_END();

    ASSERT_STDERR_CONTAINS("[transformSetParent] rejected");
    ASSERT_EQ(tman.pTransformsHot[0].parentIdx, TRANSFORM_NO_PARENT);

    free_test_manager(&man);
    free_test_transform_manager(&tman);
}

// ---------------------------------------------------------------------------
// 10. Self-parenting rejected
// ---------------------------------------------------------------------------

void test_set_parent_rejects_self(void)
{
    Models3DManager  man  = make_test_manager(1, 2);
    TransformManager tman = make_test_transform_manager(2);

    man.pDenseModelInstances[0].transformIdx = 0;
    man.pDenseModelInstances[1].transformIdx = 1;

    CAPTURE_STDERR_BEGIN();
    transformSetParent(&tman, 1, 1);
    CAPTURE_STDERR_END();

    ASSERT_STDERR_CONTAINS("[transformSetParent] rejected");
    ASSERT_EQ(tman.pTransformsHot[1].parentIdx, TRANSFORM_NO_PARENT);

    free_test_manager(&man);
    free_test_transform_manager(&tman);
}

// ---------------------------------------------------------------------------
// 11. Cross-type: light instance parented to model instance
// ---------------------------------------------------------------------------

void test_cross_type_light_parented_to_model(void)
{
    // 1 model with 2 instances, 1 light with 1 instance
    SCENE*            scene = make_test_scene_mixed(1, 2, 1, 1);
    TransformManager* tman  = &scene->transformManager;

    // Model instance 0 at (10, 0, 0)
    Model3DInstance* modelInst = &scene->modelsManager.pDenseModelInstances[0];
    transformSetPosition(&tman->pTransformsCold[modelInst->transformIdx], (vec3){10.0f, 0.0f, 0.0f});

    // Light instance 0 at local (0, 5, 0)
    LightVolumeInstance* lightInst = &scene->lightManager.pDenseLightInstances[0];
    transformSetPosition(&tman->pTransformsCold[lightInst->transformIdx], (vec3){0.0f, 5.0f, 0.0f});

    // Parent light to model instance via raw transform indices
    // Model transforms: [0, 2), Light transforms: [2, 3)
    // parent=0 (model inst 0), child=2 (light inst 0) — parent < child ✓
    transformSetParent(tman, lightInst->transformIdx, modelInst->transformIdx);

    resolveTransforms(tman);

    // Light world = parent(10,0,0) + local(0,5,0) = (10, 5, 0)
    TransformHot* lNode = &tman->pTransformsHot[lightInst->transformIdx];
    ASSERT_FLOAT_EQ(lNode->worldMatrix[3][0], 10.0f);
    ASSERT_FLOAT_EQ(lNode->worldMatrix[3][1], 5.0f);
    ASSERT_FLOAT_EQ(lNode->worldMatrix[3][2], 0.0f);

    free_test_scene_mixed(scene);
}

// ---------------------------------------------------------------------------
// 12. Cross-type: cascade-disable model parent disables light child
// ---------------------------------------------------------------------------

void test_cross_type_cascade_disable(void)
{
    SCENE*            scene = make_test_scene_mixed(1, 2, 1, 1);
    TransformManager* tman  = &scene->transformManager;

    Model3DInstance*     modelInst = &scene->modelsManager.pDenseModelInstances[0];
    LightVolumeInstance* lightInst = &scene->lightManager.pDenseLightInstances[0];

    transformSetPosition(&tman->pTransformsCold[modelInst->transformIdx], (vec3){10.0f, 0.0f, 0.0f});
    transformSetPosition(&tman->pTransformsCold[lightInst->transformIdx], (vec3){0.0f, 5.0f, 0.0f});

    transformSetParent(tman, lightInst->transformIdx, modelInst->transformIdx);

    // Disable model instance's transform
    tman->pTransformsHot[modelInst->transformIdx].lifecycle = TRANSFORM_DISABLED;

    resolveTransforms(tman);

    // Model parent is disabled → light child cascade-disabled (DIRTY sticks on disabled nodes)
    ASSERT_EQ(tman->pTransformsHot[modelInst->transformIdx].passFlags & TRANSFORM_PASS_DISABLED, TRANSFORM_PASS_DISABLED);
    ASSERT_EQ(tman->pTransformsHot[lightInst->transformIdx].passFlags & TRANSFORM_PASS_DISABLED, TRANSFORM_PASS_DISABLED);

    free_test_scene_mixed(scene);
}

// ---------------------------------------------------------------------------
// 13. Cross-type: model parent scale affects light child position
// ---------------------------------------------------------------------------

void test_cross_type_parent_scale_affects_light(void)
{
    SCENE*            scene = make_test_scene_mixed(1, 1, 1, 1);
    TransformManager* tman  = &scene->transformManager;

    Model3DInstance*     modelInst = &scene->modelsManager.pDenseModelInstances[0];
    LightVolumeInstance* lightInst = &scene->lightManager.pDenseLightInstances[0];

    // Model at origin, scale 3x
    transformSetPosition(&tman->pTransformsCold[modelInst->transformIdx], (vec3){0.0f, 0.0f, 0.0f});
    transformSetScaleUniform(&tman->pTransformsCold[modelInst->transformIdx], 3.0f);

    // Light local (2, 0, 0)
    transformSetPosition(&tman->pTransformsCold[lightInst->transformIdx], (vec3){2.0f, 0.0f, 0.0f});

    transformSetParent(tman, lightInst->transformIdx, modelInst->transformIdx);

    resolveTransforms(tman);

    // Light world = 3 * (2, 0, 0) = (6, 0, 0)
    TransformHot* lNode = &tman->pTransformsHot[lightInst->transformIdx];
    ASSERT_FLOAT_EQ(lNode->worldMatrix[3][0], 6.0f);
    ASSERT_FLOAT_EQ(lNode->worldMatrix[3][1], 0.0f);
    ASSERT_FLOAT_EQ(lNode->worldMatrix[3][2], 0.0f);

    free_test_scene_mixed(scene);
}

// ---------------------------------------------------------------------------
// 14. Cross-type: re-enable model parent restores light child
// ---------------------------------------------------------------------------

void test_cross_type_reenable_restores_light(void)
{
    SCENE*            scene = make_test_scene_mixed(1, 1, 1, 1);
    TransformManager* tman  = &scene->transformManager;

    Model3DInstance*     modelInst = &scene->modelsManager.pDenseModelInstances[0];
    LightVolumeInstance* lightInst = &scene->lightManager.pDenseLightInstances[0];

    transformSetPosition(&tman->pTransformsCold[modelInst->transformIdx], (vec3){10.0f, 0.0f, 0.0f});
    transformSetPosition(&tman->pTransformsCold[lightInst->transformIdx], (vec3){0.0f, 5.0f, 0.0f});

    transformSetParent(tman, lightInst->transformIdx, modelInst->transformIdx);

    // Frame 1: disable parent
    tman->pTransformsHot[modelInst->transformIdx].lifecycle = TRANSFORM_DISABLED;
    resolveTransforms(tman);
    ASSERT_EQ(tman->pTransformsHot[lightInst->transformIdx].passFlags & TRANSFORM_PASS_DISABLED, TRANSFORM_PASS_DISABLED);

    // Frame 2: re-enable parent
    tman->pTransformsHot[modelInst->transformIdx].lifecycle = TRANSFORM_ALIVE;
    resolveTransforms(tman);
    ASSERT_EQ(tman->pTransformsHot[lightInst->transformIdx].passFlags, 0);

    // Light world position restored: (10, 5, 0)
    TransformHot* lNode = &tman->pTransformsHot[lightInst->transformIdx];
    ASSERT_FLOAT_EQ(lNode->worldMatrix[3][0], 10.0f);
    ASSERT_FLOAT_EQ(lNode->worldMatrix[3][1], 5.0f);
    ASSERT_FLOAT_EQ(lNode->worldMatrix[3][2], 0.0f);

    free_test_scene_mixed(scene);
}

// ---------------------------------------------------------------------------
// 15. Cross-type: clear parent makes light a root again
// ---------------------------------------------------------------------------

void test_cross_type_clear_parent(void)
{
    SCENE*            scene = make_test_scene_mixed(1, 1, 1, 1);
    TransformManager* tman  = &scene->transformManager;

    Model3DInstance*     modelInst = &scene->modelsManager.pDenseModelInstances[0];
    LightVolumeInstance* lightInst = &scene->lightManager.pDenseLightInstances[0];

    transformSetPosition(&tman->pTransformsCold[modelInst->transformIdx], (vec3){10.0f, 0.0f, 0.0f});
    transformSetPosition(&tman->pTransformsCold[lightInst->transformIdx], (vec3){0.0f, 5.0f, 0.0f});

    transformSetParent(tman, lightInst->transformIdx, modelInst->transformIdx);

    // Frame 1: parented → light at (10, 5, 0)
    resolveTransforms(tman);
    ASSERT_FLOAT_EQ(tman->pTransformsHot[lightInst->transformIdx].worldMatrix[3][0], 10.0f);
    ASSERT_FLOAT_EQ(tman->pTransformsHot[lightInst->transformIdx].worldMatrix[3][1], 5.0f);

    // Clear parent
    transformClearParent(tman, lightInst->transformIdx);

    // Frame 2: unparented → light at (0, 5, 0)
    resolveTransforms(tman);
    ASSERT_FLOAT_EQ(tman->pTransformsHot[lightInst->transformIdx].worldMatrix[3][0], 0.0f);
    ASSERT_FLOAT_EQ(tman->pTransformsHot[lightInst->transformIdx].worldMatrix[3][1], 5.0f);
    ASSERT_FLOAT_EQ(tman->pTransformsHot[lightInst->transformIdx].worldMatrix[3][2], 0.0f);

    free_test_scene_mixed(scene);
}

// ---------------------------------------------------------------------------
// 16. Cross-type UBO: parented light produces correct UBO output
// ---------------------------------------------------------------------------

void test_cross_type_ubo_model_and_light(void)
{
    // 1 model with 1 instance, 1 light with 1 instance
    SCENE*            scene = make_test_scene_mixed(1, 1, 1, 1);
    GraphicSwapchain  sc    = make_fake_swapchain();
    Camera3D          cam   = make_test_camera();
    TransformManager* tman  = &scene->transformManager;

    Model3DInstance*     modelInst = &scene->modelsManager.pDenseModelInstances[0];
    LightVolumeInstance* lightInst = &scene->lightManager.pDenseLightInstances[0];

    transformSetPosition(&tman->pTransformsCold[modelInst->transformIdx], (vec3){10.0f, 0.0f, 0.0f});
    transformSetPosition(&tman->pTransformsCold[lightInst->transformIdx], (vec3){0.0f, 5.0f, 0.0f});

    transformSetParent(tman, lightInst->transformIdx, modelInst->transformIdx);

    resolveTransforms(tman);

    // Model UBO fill — model instance should produce 1 entry
    ModelInstanceUBO* modelUBO = (ModelInstanceUBO*) malloc(sizeof(ModelInstanceUBO) * scene->modelsManager.modelInstanceCapacity);
    updateModelUniformBuffersDirect(modelUBO, scene);
    ASSERT_EQ(scene->modelsManager.modelInstanceLen, 1);
    // Model matrix should contain model's world position (10,0,0)
    ASSERT_FLOAT_EQ(modelUBO[0].model[3][0], 10.0f);
    ASSERT_FLOAT_EQ(modelUBO[0].model[3][1], 0.0f);

    // Light UBO fill — light instance should produce 1 entry
    LightInstanceUBO* lightUBO = (LightInstanceUBO*) malloc(sizeof(LightInstanceUBO) * scene->lightManager.lightInstanceCapacity);
    updateLightUniformBuffersDirect(lightUBO, 0.0f, scene);
    ASSERT_EQ(scene->lightManager.lightInstanceCount, 1);
    // Light model matrix should contain light's world position (10,5,0)
    ASSERT_FLOAT_EQ(lightUBO[0].model[3][0], 10.0f);
    ASSERT_FLOAT_EQ(lightUBO[0].model[3][1], 5.0f);

    free(modelUBO);
    free(lightUBO);
    free_test_scene_mixed(scene);
}

// ---------------------------------------------------------------------------
// 17. Cross-type UBO: disabled model parent → both model and light UBO entries zero
// ---------------------------------------------------------------------------

void test_cross_type_ubo_cascade_disable_both(void)
{
    SCENE*            scene = make_test_scene_mixed(1, 1, 1, 1);
    GraphicSwapchain  sc    = make_fake_swapchain();
    Camera3D          cam   = make_test_camera();
    TransformManager* tman  = &scene->transformManager;

    Model3DInstance*     modelInst = &scene->modelsManager.pDenseModelInstances[0];
    LightVolumeInstance* lightInst = &scene->lightManager.pDenseLightInstances[0];

    transformSetPosition(&tman->pTransformsCold[modelInst->transformIdx], (vec3){10.0f, 0.0f, 0.0f});
    transformSetPosition(&tman->pTransformsCold[lightInst->transformIdx], (vec3){0.0f, 5.0f, 0.0f});

    transformSetParent(tman, lightInst->transformIdx, modelInst->transformIdx);

    // Disable model instance + its transform
    modelInst->lifecycle = INSTANCE_DISABLED;
    tman->pTransformsHot[modelInst->transformIdx].lifecycle = TRANSFORM_DISABLED;

    resolveTransforms(tman);

    ModelInstanceUBO* modelUBO = (ModelInstanceUBO*) malloc(sizeof(ModelInstanceUBO) * scene->modelsManager.modelInstanceCapacity);
    updateModelUniformBuffersDirect(modelUBO, scene);
    ASSERT_EQ(scene->modelsManager.modelInstanceLen, 0);

    LightInstanceUBO* lightUBO = (LightInstanceUBO*) malloc(sizeof(LightInstanceUBO) * scene->lightManager.lightInstanceCapacity);
    updateLightUniformBuffersDirect(lightUBO, 0.0f, scene);
    ASSERT_EQ(scene->lightManager.lightInstanceCount, 0);

    free(modelUBO);
    free(lightUBO);
    free_test_scene_mixed(scene);
}

// ---------------------------------------------------------------------------
// 18. Cross-type UBO: move model parent → light UBO reflects new world position
// ---------------------------------------------------------------------------

void test_cross_type_ubo_parent_move_updates_light(void)
{
    SCENE*            scene = make_test_scene_mixed(1, 1, 1, 1);
    GraphicSwapchain  sc    = make_fake_swapchain();
    Camera3D          cam   = make_test_camera();
    TransformManager* tman  = &scene->transformManager;

    Model3DInstance*     modelInst = &scene->modelsManager.pDenseModelInstances[0];
    LightVolumeInstance* lightInst = &scene->lightManager.pDenseLightInstances[0];

    transformSetPosition(&tman->pTransformsCold[modelInst->transformIdx], (vec3){0.0f, 0.0f, 0.0f});
    transformSetPosition(&tman->pTransformsCold[lightInst->transformIdx], (vec3){0.0f, 5.0f, 0.0f});

    transformSetParent(tman, lightInst->transformIdx, modelInst->transformIdx);

    // Frame 1: model at origin
    resolveTransforms(tman);
    LightInstanceUBO* lightUBO1 = (LightInstanceUBO*) malloc(sizeof(LightInstanceUBO) * scene->lightManager.lightInstanceCapacity);
    updateLightUniformBuffersDirect(lightUBO1, 0.0f, scene);
    ASSERT_FLOAT_EQ(lightUBO1[0].model[3][0], 0.0f);
    ASSERT_FLOAT_EQ(lightUBO1[0].model[3][1], 5.0f);
    free(lightUBO1);

    // Frame 2: move model to (20, 0, 0) — light should follow
    transformSetPosition(&tman->pTransformsCold[modelInst->transformIdx], (vec3){20.0f, 0.0f, 0.0f});
    transformMarkDirty(tman, modelInst->transformIdx);
    resolveTransforms(tman);
    LightInstanceUBO* lightUBO2 = (LightInstanceUBO*) malloc(sizeof(LightInstanceUBO) * scene->lightManager.lightInstanceCapacity);
    updateLightUniformBuffersDirect(lightUBO2, 0.016f, scene);
    ASSERT_FLOAT_EQ(lightUBO2[0].model[3][0], 20.0f);
    ASSERT_FLOAT_EQ(lightUBO2[0].model[3][1], 5.0f);
    free(lightUBO2);

    free_test_scene_mixed(scene);
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
    RUN_TEST(test_cross_type_light_parented_to_model);
    RUN_TEST(test_cross_type_cascade_disable);
    RUN_TEST(test_cross_type_parent_scale_affects_light);
    RUN_TEST(test_cross_type_reenable_restores_light);
    RUN_TEST(test_cross_type_clear_parent);
    RUN_TEST(test_cross_type_ubo_model_and_light);
    RUN_TEST(test_cross_type_ubo_cascade_disable_both);
    RUN_TEST(test_cross_type_ubo_parent_move_updates_light);

    TEST_SUMMARY();
}
