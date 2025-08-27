#pragma once
#include "cam.h"

typedef struct Camera3D
{
    vec3  eye;
    float _pad1; // explicit padding
    vec3  center;
    float _pad2; // explicit padding
    vec3  up;
    float _pad3; // explicit padding
    vec2  screenSize;
    float _pad4[2];
} Camera3D;

void updateCamera(Camera3D* camera);
