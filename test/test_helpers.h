#pragma once
#include "scene.h"
#include <stdint.h>

// Creates a Models3DManager with `modelCount` models, each having `instancesPerModel` instances.
// All instances alive=INSTANCE_ALIVE, all models alive=MODEL_ALIVE.
// instanceOffset array allocated and filled with cumulative sums.
Models3DManager make_test_manager(uint32_t modelCount, uint32_t instancesPerModel);

// Frees everything allocated by make_test_manager.
void free_test_manager(Models3DManager* man);

// Creates a LightManager with `lightCount` lights, each having `instancesPerLight` instances.
// All instances alive=LIGHT_INSTANCE_ALIVE, all lights alive=LIGHT_ALIVE.
// instanceOffset array allocated and filled with cumulative sums.
LightManager make_test_light_manager(uint32_t lightCount, uint32_t instancesPerLight);

// Frees everything allocated by make_test_light_manager.
void free_test_light_manager(LightManager* man);

// Creates a minimal SCENE suitable for updateModelUniformBuffers tests.
// Models have identity matrices on all instances.
// Camera at (0,0,5) looking at origin.
// Light/particle managers zeroed out.
SCENE* make_test_scene(uint32_t modelCount, uint32_t instancesPerModel);

// Frees everything allocated by make_test_scene.
void free_test_scene(SCENE* scene);
