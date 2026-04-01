#include "test_harness.h"
#include "test_helpers.h"

// ---------------------------------------------------------------------------
// Instance-level tests
// ---------------------------------------------------------------------------

void test_destroy_light_instance_removes_it(void)
{
    LightManager man = make_test_light_manager(1, 4);

    destroyLightInstance(&man, 0, 0);

    ASSERT_EQ(man.pLight[0].pLightVolumeInstance[0].alive, LIGHT_INSTANCE_DESTROYED);
    ASSERT_EQ(man.pLight[0].pLightVolumeInstance[1].alive, LIGHT_INSTANCE_ALIVE);

    free_test_light_manager(&man);
}

void test_enable_destroyed_light_instance_refused(void)
{
    LightManager man = make_test_light_manager(1, 4);

    destroyLightInstance(&man, 0, 0);

    CAPTURE_STDERR_BEGIN();
    enableLightInstance(&man, 0, 0);
    CAPTURE_STDERR_END();

    ASSERT_EQ(man.pLight[0].pLightVolumeInstance[0].alive, LIGHT_INSTANCE_DESTROYED);
    ASSERT_STDERR_CONTAINS("[enableLightInstance] instance 0 of light 0 is permanently destroyed");

    free_test_light_manager(&man);
}

void test_destroy_light_instance_idempotent(void)
{
    LightManager man = make_test_light_manager(1, 4);

    destroyLightInstance(&man, 0, 0);

    CAPTURE_STDERR_BEGIN();
    destroyLightInstance(&man, 0, 0);
    CAPTURE_STDERR_END();

    ASSERT_EQ(man.pLight[0].pLightVolumeInstance[0].alive, LIGHT_INSTANCE_DESTROYED);
    ASSERT_STDERR_CONTAINS("[destroyLightInstance] instance 0 of light 0 is already destroyed");

    free_test_light_manager(&man);
}

void test_destroy_light_instance_survives_light_toggle(void)
{
    LightManager man = make_test_light_manager(1, 4);

    destroyLightInstance(&man, 0, 0);
    disableLight(&man, 0);
    enableLight(&man, 0);

    ASSERT_EQ(man.pLight[0].pLightVolumeInstance[0].alive, LIGHT_INSTANCE_DESTROYED);
    ASSERT_EQ(man.pLight[0].pLightVolumeInstance[1].alive, LIGHT_INSTANCE_ALIVE);

    free_test_light_manager(&man);
}

void test_destroy_light_instance_while_light_disabled(void)
{
    LightManager man = make_test_light_manager(1, 4);

    disableLight(&man, 0);
    destroyLightInstance(&man, 0, 0);
    enableLight(&man, 0);

    ASSERT_EQ(man.pLight[0].pLightVolumeInstance[0].alive, LIGHT_INSTANCE_DESTROYED);
    ASSERT_EQ(man.pLight[0].pLightVolumeInstance[1].alive, LIGHT_INSTANCE_ALIVE);

    free_test_light_manager(&man);
}

void test_destroy_light_instance_then_destroy_light(void)
{
    LightManager man = make_test_light_manager(1, 4);

    destroyLightInstance(&man, 0, 0);
    destroyLight(&man, 0);

    ASSERT_EQ(man.pLight[0].alive, LIGHT_DESTROYED);
    ASSERT_NULL(man.pLight[0].pLightVolumeInstance);

    free_test_light_manager(&man);
}

void test_destroyed_light_refuses_all_operations(void)
{
    LightManager man = make_test_light_manager(1, 4);

    destroyLight(&man, 0);

    CAPTURE_STDERR_BEGIN();
    enableLight(&man, 0);
    disableLight(&man, 0);
    enableLightInstance(&man, 0, 0);
    destroyLightInstance(&man, 0, 0);
    CAPTURE_STDERR_END();

    ASSERT_EQ(man.pLight[0].alive, LIGHT_DESTROYED);

    free_test_light_manager(&man);
}

void test_disable_then_destroy_light_instance(void)
{
    LightManager man = make_test_light_manager(1, 4);

    disableLightInstance(&man, 0, 0);
    ASSERT_EQ(man.pLight[0].pLightVolumeInstance[0].alive, LIGHT_INSTANCE_DISABLED);

    destroyLightInstance(&man, 0, 0);
    ASSERT_EQ(man.pLight[0].pLightVolumeInstance[0].alive, LIGHT_INSTANCE_DESTROYED);

    free_test_light_manager(&man);
}

void test_enable_after_disable_then_destroy_light_refused(void)
{
    LightManager man = make_test_light_manager(1, 4);

    disableLightInstance(&man, 0, 0);
    destroyLightInstance(&man, 0, 0);

    CAPTURE_STDERR_BEGIN();
    enableLightInstance(&man, 0, 0);
    CAPTURE_STDERR_END();

    ASSERT_EQ(man.pLight[0].pLightVolumeInstance[0].alive, LIGHT_INSTANCE_DESTROYED);
    ASSERT_STDERR_CONTAINS("[enableLightInstance] instance 0 of light 0 is permanently destroyed");

    free_test_light_manager(&man);
}

void test_disable_enable_light_instance_roundtrip(void)
{
    LightManager man = make_test_light_manager(1, 4);

    disableLightInstance(&man, 0, 0);
    ASSERT_EQ(man.pLight[0].pLightVolumeInstance[0].alive, LIGHT_INSTANCE_DISABLED);

    enableLightInstance(&man, 0, 0);
    ASSERT_EQ(man.pLight[0].pLightVolumeInstance[0].alive, LIGHT_INSTANCE_ALIVE);

    free_test_light_manager(&man);
}

// ---------------------------------------------------------------------------
// Light-level tests
// ---------------------------------------------------------------------------

void test_disable_enable_light_roundtrip(void)
{
    LightManager man = make_test_light_manager(1, 4);

    disableLight(&man, 0);
    ASSERT_EQ(man.pLight[0].alive, LIGHT_DISABLED);

    enableLight(&man, 0);
    ASSERT_EQ(man.pLight[0].alive, LIGHT_ALIVE);

    free_test_light_manager(&man);
}

// ---------------------------------------------------------------------------
// Bounds safety
// ---------------------------------------------------------------------------

void test_out_of_range_light_index_no_crash(void)
{
    LightManager man = make_test_light_manager(1, 4);

    CAPTURE_STDERR_BEGIN();
    disableLightInstance(&man, 99, 0);
    enableLightInstance(&man, 99, 0);
    destroyLightInstance(&man, 99, 0);
    destroyLight(&man, 99);
    CAPTURE_STDERR_END();

    free_test_light_manager(&man);
}

void test_out_of_range_light_instance_index_no_crash(void)
{
    LightManager man = make_test_light_manager(1, 4);

    CAPTURE_STDERR_BEGIN();
    disableLightInstance(&man, 0, 99);
    enableLightInstance(&man, 0, 99);
    destroyLightInstance(&man, 0, 99);
    CAPTURE_STDERR_END();

    free_test_light_manager(&man);
}

// ---------------------------------------------------------------------------
// Query tests
// ---------------------------------------------------------------------------

void test_query_light_instance_alive_states(void)
{
    LightManager man = make_test_light_manager(1, 4);

    ASSERT_EQ(queryLightInstanceAlive(&man, 0, 0), LIGHT_INSTANCE_ALIVE);

    disableLightInstance(&man, 0, 0);
    ASSERT_EQ(queryLightInstanceAlive(&man, 0, 0), LIGHT_INSTANCE_DISABLED);

    enableLightInstance(&man, 0, 0);
    ASSERT_EQ(queryLightInstanceAlive(&man, 0, 0), LIGHT_INSTANCE_ALIVE);

    destroyLightInstance(&man, 0, 0);
    ASSERT_EQ(queryLightInstanceAlive(&man, 0, 0), LIGHT_INSTANCE_DESTROYED);

    // Out of range returns -1
    ASSERT_EQ(queryLightInstanceAlive(&man, 99, 0), -1);
    ASSERT_EQ(queryLightInstanceAlive(&man, 0, 99), -1);

    free_test_light_manager(&man);
}

void test_query_light_instance_alive_after_light_destroyed(void)
{
    LightManager man = make_test_light_manager(1, 4);

    destroyLight(&man, 0);

    ASSERT_EQ(queryLightInstanceAlive(&man, 0, 0), -1);

    int state = queryLightInstanceAlive(&man, 0, 0);
    if (state == LIGHT_INSTANCE_ALIVE)
    {
        disableLightInstance(&man, 0, 0);
    }
    else
    {
        CAPTURE_STDERR_BEGIN();
        enableLightInstance(&man, 0, 0);
        CAPTURE_STDERR_END();
        ASSERT_STDERR_CONTAINS("[enableLightInstance] light 0 is permanently destroyed");
    }

    free_test_light_manager(&man);
}

// ---------------------------------------------------------------------------
// Multi-light independence
// ---------------------------------------------------------------------------

void test_multi_light_independence(void)
{
    LightManager man = make_test_light_manager(3, 4);

    destroyLightInstance(&man, 0, 0);
    disableLight(&man, 1);

    // Light 0: instance 0 destroyed, rest alive
    ASSERT_EQ(man.pLight[0].pLightVolumeInstance[0].alive, LIGHT_INSTANCE_DESTROYED);
    ASSERT_EQ(man.pLight[0].pLightVolumeInstance[1].alive, LIGHT_INSTANCE_ALIVE);
    // Light 1: disabled but instances untouched
    ASSERT_EQ(man.pLight[1].alive, LIGHT_DISABLED);
    ASSERT_EQ(man.pLight[1].pLightVolumeInstance[0].alive, LIGHT_INSTANCE_ALIVE);
    // Light 2: completely untouched
    ASSERT_EQ(man.pLight[2].alive, LIGHT_ALIVE);
    ASSERT_EQ(man.pLight[2].pLightVolumeInstance[0].alive, LIGHT_INSTANCE_ALIVE);

    free_test_light_manager(&man);
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main(void)
{
    printf("=== Light Lifecycle Tests ===\n");

    RUN_TEST(test_destroy_light_instance_removes_it);
    RUN_TEST(test_enable_destroyed_light_instance_refused);
    RUN_TEST(test_destroy_light_instance_idempotent);
    RUN_TEST(test_destroy_light_instance_survives_light_toggle);
    RUN_TEST(test_destroy_light_instance_while_light_disabled);
    RUN_TEST(test_destroy_light_instance_then_destroy_light);
    RUN_TEST(test_destroyed_light_refuses_all_operations);
    RUN_TEST(test_disable_then_destroy_light_instance);
    RUN_TEST(test_enable_after_disable_then_destroy_light_refused);
    RUN_TEST(test_disable_enable_light_instance_roundtrip);
    RUN_TEST(test_disable_enable_light_roundtrip);
    RUN_TEST(test_out_of_range_light_index_no_crash);
    RUN_TEST(test_out_of_range_light_instance_index_no_crash);
    RUN_TEST(test_query_light_instance_alive_states);
    RUN_TEST(test_query_light_instance_alive_after_light_destroyed);
    RUN_TEST(test_multi_light_independence);

    TEST_SUMMARY();
}
