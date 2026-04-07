#include "test_harness.h"
#include "test_helpers.h"
#include "model.h"
#include "scene.h"

// ---------------------------------------------------------------------------
// Instance destruction
// ---------------------------------------------------------------------------

void test_destroy_instance_removes_it(void)
{
    Models3DManager man = make_test_manager(1, 4);

    CAPTURE_STDERR_BEGIN();
    destroyInstance(&man, 0, 0);
    CAPTURE_STDERR_END();

    ASSERT_EQ(man.pDenseModelInstances[man.pModel[0].denseBaseOffset + 0].alive, INSTANCE_DESTROYED);
    ASSERT_EQ(man.pDenseModelInstances[man.pModel[0].denseBaseOffset + 1].alive, INSTANCE_ALIVE);
    ASSERT_EQ(man.pDenseModelInstances[man.pModel[0].denseBaseOffset + 2].alive, INSTANCE_ALIVE);
    ASSERT_EQ(man.pDenseModelInstances[man.pModel[0].denseBaseOffset + 3].alive, INSTANCE_ALIVE);
    ASSERT_STDERR_EMPTY();

    free_test_manager(&man);
}

void test_enable_destroyed_instance_refused(void)
{
    Models3DManager man = make_test_manager(1, 4);

    destroyInstance(&man, 0, 0);

    CAPTURE_STDERR_BEGIN();
    enableInstance(&man, 0, 0);
    CAPTURE_STDERR_END();

    ASSERT_EQ(man.pDenseModelInstances[man.pModel[0].denseBaseOffset + 0].alive, INSTANCE_DESTROYED);
    ASSERT_STDERR_CONTAINS("[enableInstance] instance 0 of model 0 is permanently destroyed");

    free_test_manager(&man);
}

void test_destroy_instance_idempotent(void)
{
    Models3DManager man = make_test_manager(1, 4);

    destroyInstance(&man, 0, 0);

    CAPTURE_STDERR_BEGIN();
    destroyInstance(&man, 0, 0);
    CAPTURE_STDERR_END();

    ASSERT_EQ(man.pDenseModelInstances[man.pModel[0].denseBaseOffset + 0].alive, INSTANCE_DESTROYED);
    ASSERT_STDERR_CONTAINS("[destroyInstance] instance 0 of model 0 is already destroyed");

    free_test_manager(&man);
}

// ---------------------------------------------------------------------------
// Instance destruction + model toggle interaction
// ---------------------------------------------------------------------------

void test_destroy_instance_survives_model_toggle(void)
{
    Models3DManager man = make_test_manager(1, 4);

    destroyInstance(&man, 0, 0);
    disableModel(&man, 0);
    enableModel(&man, 0);

    ASSERT_EQ(man.pDenseModelInstances[man.pModel[0].denseBaseOffset + 0].alive, INSTANCE_DESTROYED);
    ASSERT_EQ(man.pDenseModelInstances[man.pModel[0].denseBaseOffset + 1].alive, INSTANCE_ALIVE);

    free_test_manager(&man);
}

void test_destroy_instance_while_model_disabled(void)
{
    Models3DManager man = make_test_manager(1, 4);

    disableModel(&man, 0);
    destroyInstance(&man, 0, 0);
    enableModel(&man, 0);

    ASSERT_EQ(man.pDenseModelInstances[man.pModel[0].denseBaseOffset + 0].alive, INSTANCE_DESTROYED);
    ASSERT_EQ(man.pDenseModelInstances[man.pModel[0].denseBaseOffset + 1].alive, INSTANCE_ALIVE);

    free_test_manager(&man);
}

// ---------------------------------------------------------------------------
// Instance destruction + model destruction
// ---------------------------------------------------------------------------

void test_destroy_instance_then_destroy_model(void)
{
    Models3DManager man = make_test_manager(1, 4);

    destroyInstance(&man, 0, 0);
    destroyModel(&man, 0);

    ASSERT_EQ(man.pModel[0].alive, MODEL_DESTROYED);

    free_test_manager(&man);
}

void test_destroyed_model_refuses_all_operations(void)
{
    Models3DManager man = make_test_manager(1, 4);

    destroyModel(&man, 0);

    CAPTURE_STDERR_BEGIN();
    enableModel(&man, 0);
    CAPTURE_STDERR_END();
    ASSERT_EQ(man.pModel[0].alive, MODEL_DESTROYED);
    ASSERT_STDERR_CONTAINS("[enableModel] model 0 is permanently destroyed");

    CAPTURE_STDERR_BEGIN();
    disableModel(&man, 0);
    CAPTURE_STDERR_END();
    ASSERT_EQ(man.pModel[0].alive, MODEL_DESTROYED);
    ASSERT_STDERR_CONTAINS("[disableModel] model 0 is permanently destroyed");

    CAPTURE_STDERR_BEGIN();
    enableInstance(&man, 0, 0);
    CAPTURE_STDERR_END();
    ASSERT_STDERR_CONTAINS("[enableInstance] model 0 is permanently destroyed");

    CAPTURE_STDERR_BEGIN();
    destroyInstance(&man, 0, 0);
    CAPTURE_STDERR_END();
    ASSERT_STDERR_CONTAINS("[destroyInstance] model 0 is permanently destroyed");

    free_test_manager(&man);
}

// ---------------------------------------------------------------------------
// Disable → destroy transition
// ---------------------------------------------------------------------------

void test_disable_then_destroy_instance(void)
{
    Models3DManager man = make_test_manager(1, 4);

    disableInstance(&man, 0, 0);
    ASSERT_EQ(man.pDenseModelInstances[man.pModel[0].denseBaseOffset + 0].alive, INSTANCE_DISABLED);

    destroyInstance(&man, 0, 0);
    ASSERT_EQ(man.pDenseModelInstances[man.pModel[0].denseBaseOffset + 0].alive, INSTANCE_DESTROYED);

    free_test_manager(&man);
}

void test_enable_after_disable_then_destroy_refused(void)
{
    Models3DManager man = make_test_manager(1, 4);

    disableInstance(&man, 0, 0);
    destroyInstance(&man, 0, 0);

    CAPTURE_STDERR_BEGIN();
    enableInstance(&man, 0, 0);
    CAPTURE_STDERR_END();

    ASSERT_EQ(man.pDenseModelInstances[man.pModel[0].denseBaseOffset + 0].alive, INSTANCE_DESTROYED);
    ASSERT_STDERR_CONTAINS("[enableInstance] instance 0 of model 0 is permanently destroyed");

    free_test_manager(&man);
}

// ---------------------------------------------------------------------------
// Basic roundtrips
// ---------------------------------------------------------------------------

void test_disable_enable_instance_roundtrip(void)
{
    Models3DManager man = make_test_manager(1, 4);

    CAPTURE_STDERR_BEGIN();
    disableInstance(&man, 0, 0);
    enableInstance(&man, 0, 0);
    CAPTURE_STDERR_END();

    ASSERT_EQ(man.pDenseModelInstances[man.pModel[0].denseBaseOffset + 0].alive, INSTANCE_ALIVE);
    ASSERT_STDERR_EMPTY();

    free_test_manager(&man);
}

void test_disable_enable_model_roundtrip(void)
{
    Models3DManager man = make_test_manager(1, 4);

    CAPTURE_STDERR_BEGIN();
    disableModel(&man, 0);
    enableModel(&man, 0);
    CAPTURE_STDERR_END();

    ASSERT_EQ(man.pModel[0].alive, MODEL_ALIVE);
    ASSERT_STDERR_EMPTY();

    free_test_manager(&man);
}

// ---------------------------------------------------------------------------
// Boundary / error cases
// ---------------------------------------------------------------------------

void test_out_of_range_model_index_no_crash(void)
{
    Models3DManager man = make_test_manager(1, 4);

    CAPTURE_STDERR_BEGIN();
    disableInstance(&man, 99, 0);
    CAPTURE_STDERR_END();
    ASSERT_STDERR_CONTAINS("[disableInstance] modelIndex 99 out of range");

    CAPTURE_STDERR_BEGIN();
    enableInstance(&man, 99, 0);
    CAPTURE_STDERR_END();
    ASSERT_STDERR_CONTAINS("[enableInstance] modelIndex 99 out of range");

    CAPTURE_STDERR_BEGIN();
    destroyInstance(&man, 99, 0);
    CAPTURE_STDERR_END();
    ASSERT_STDERR_CONTAINS("[destroyInstance] modelIndex 99 out of range");

    CAPTURE_STDERR_BEGIN();
    destroyModel(&man, 99);
    CAPTURE_STDERR_END();
    ASSERT_STDERR_CONTAINS("[destroyModel] modelIndex 99 out of range");

    // Original state untouched
    ASSERT_EQ(man.pModel[0].alive, MODEL_ALIVE);
    ASSERT_EQ(man.pDenseModelInstances[man.pModel[0].denseBaseOffset + 0].alive, INSTANCE_ALIVE);

    free_test_manager(&man);
}

void test_out_of_range_instance_index_no_crash(void)
{
    Models3DManager man = make_test_manager(1, 4);

    CAPTURE_STDERR_BEGIN();
    disableInstance(&man, 0, 99);
    CAPTURE_STDERR_END();
    ASSERT_STDERR_CONTAINS("[disableInstance] instanceIndex 99 out of range");

    CAPTURE_STDERR_BEGIN();
    enableInstance(&man, 0, 99);
    CAPTURE_STDERR_END();
    ASSERT_STDERR_CONTAINS("[enableInstance] instanceIndex 99 out of range");

    CAPTURE_STDERR_BEGIN();
    destroyInstance(&man, 0, 99);
    CAPTURE_STDERR_END();
    ASSERT_STDERR_CONTAINS("[destroyInstance] instanceIndex 99 out of range");

    ASSERT_EQ(man.pDenseModelInstances[man.pModel[0].denseBaseOffset + 0].alive, INSTANCE_ALIVE);

    free_test_manager(&man);
}

// ---------------------------------------------------------------------------
// Multi-model independence
// ---------------------------------------------------------------------------

void test_multi_model_independence(void)
{
    Models3DManager man = make_test_manager(3, 4);

    destroyInstance(&man, 0, 0);
    disableModel(&man, 1);

    // Model 0: instance 0 destroyed, rest alive
    ASSERT_EQ(man.pDenseModelInstances[man.pModel[0].denseBaseOffset + 0].alive, INSTANCE_DESTROYED);
    ASSERT_EQ(man.pDenseModelInstances[man.pModel[0].denseBaseOffset + 1].alive, INSTANCE_ALIVE);
    ASSERT_EQ(man.pModel[0].alive, MODEL_ALIVE);

    // Model 1: disabled, instances untouched
    ASSERT_EQ(man.pModel[1].alive, MODEL_DISABLED);
    ASSERT_EQ(man.pDenseModelInstances[man.pModel[1].denseBaseOffset + 0].alive, INSTANCE_ALIVE);

    // Model 2: completely untouched
    ASSERT_EQ(man.pModel[2].alive, MODEL_ALIVE);
    ASSERT_EQ(man.pDenseModelInstances[man.pModel[2].denseBaseOffset + 0].alive, INSTANCE_ALIVE);

    free_test_manager(&man);
}

// ---------------------------------------------------------------------------
// Access pattern: mirrors demo keybind code that queries instance state
// before calling guarded API functions
// ---------------------------------------------------------------------------

void test_query_instance_alive_after_model_destroyed(void)
{
    Models3DManager man = make_test_manager(1, 4);

    destroyModel(&man, 0);

    // queryInstanceAlive must return -1 (model is destroyed)
    ASSERT_EQ(queryInstanceAlive(&man, 0, 0), -1);

    // Mirrors demo toggle pattern: query state, then call API
    int state = queryInstanceAlive(&man, 0, 0);
    if (state == INSTANCE_ALIVE)
    {
        disableInstance(&man, 0, 0);
    }
    else
    {
        CAPTURE_STDERR_BEGIN();
        enableInstance(&man, 0, 0);
        CAPTURE_STDERR_END();
        ASSERT_STDERR_CONTAINS("[enableInstance] model 0 is permanently destroyed");
    }

    free_test_manager(&man);
}

void test_query_instance_alive_states(void)
{
    Models3DManager man = make_test_manager(1, 4);

    ASSERT_EQ(queryInstanceAlive(&man, 0, 0), INSTANCE_ALIVE);

    disableInstance(&man, 0, 0);
    ASSERT_EQ(queryInstanceAlive(&man, 0, 0), INSTANCE_DISABLED);

    enableInstance(&man, 0, 0);
    ASSERT_EQ(queryInstanceAlive(&man, 0, 0), INSTANCE_ALIVE);

    destroyInstance(&man, 0, 0);
    ASSERT_EQ(queryInstanceAlive(&man, 0, 0), INSTANCE_DESTROYED);

    // Out of range returns -1
    ASSERT_EQ(queryInstanceAlive(&man, 99, 0), -1);
    ASSERT_EQ(queryInstanceAlive(&man, 0, 99), -1);

    free_test_manager(&man);
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main(void)
{
    printf("=== Scene Lifecycle Tests ===\n");

    RUN_TEST(test_destroy_instance_removes_it);
    RUN_TEST(test_enable_destroyed_instance_refused);
    RUN_TEST(test_destroy_instance_idempotent);
    RUN_TEST(test_destroy_instance_survives_model_toggle);
    RUN_TEST(test_destroy_instance_while_model_disabled);
    RUN_TEST(test_destroy_instance_then_destroy_model);
    RUN_TEST(test_destroyed_model_refuses_all_operations);
    RUN_TEST(test_disable_then_destroy_instance);
    RUN_TEST(test_enable_after_disable_then_destroy_refused);
    RUN_TEST(test_disable_enable_instance_roundtrip);
    RUN_TEST(test_disable_enable_model_roundtrip);
    RUN_TEST(test_out_of_range_model_index_no_crash);
    RUN_TEST(test_out_of_range_instance_index_no_crash);
    RUN_TEST(test_query_instance_alive_after_model_destroyed);
    RUN_TEST(test_query_instance_alive_states);
    RUN_TEST(test_multi_model_independence);

    TEST_SUMMARY();
}
