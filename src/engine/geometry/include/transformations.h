#pragma once
#include "mat4.h"
typedef struct MODEL_VIEW_PROJECTION {
    mat4 model;
    mat4 view;
    mat4 proj;
} MODEL_VIEW_PROJECTION;
typedef struct UboDeltaTime {
    float deltaTime;
} UboDeltaTime;
