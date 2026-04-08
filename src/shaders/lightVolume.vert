#version 450

// Per-frame globals (binding 9) — view/proj computed once per frame
layout(binding = 9) uniform FrameGlobals {
    mat4 view;
    mat4 proj;
    mat4 viewProj;
    mat4 invViewProj;
    vec3 cameraEye;
    float _pad1;
    vec3 cameraCenter;
    float _pad2;
    vec3 cameraUp;
    float _pad3;
    vec2 screenSize;
    vec2 _pad4;
} globals;

struct LightInstanceData {
    mat4 model;
    vec4 color;
    vec3 position;
    float scale;
};

layout(std430, binding = 7) buffer LightInstances {
    LightInstanceData instances[];
} lights;

layout(location = 0) in vec3 point;
layout(location = 1) in vec3 barycentric;
layout(location = 0) out vec3 pointOut;
layout(location = 1) out vec3 baryOut;
layout(location = 2) out flat int instanceIndex;

void main() {
    gl_Position = globals.proj * globals.view * lights.instances[gl_InstanceIndex].model * vec4(point, 1.0);
    pointOut = point;
    baryOut = barycentric;
    instanceIndex = gl_InstanceIndex;
}
