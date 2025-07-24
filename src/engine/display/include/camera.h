#pragma once
#include "cam.h"

typedef struct Camera3D {
    vec3 eye;
    vec3 center;
    vec3 up;
    float cameraAngle;
} Camera3D;

void updateCamera(Camera3D* camera);
