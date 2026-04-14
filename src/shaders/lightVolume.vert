#version 450
#extension GL_EXT_nonuniform_qualifier : require

layout(set = 1, binding = 0) uniform FrameGlobals {
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

layout(std430, set = 0, binding = 1) readonly buffer LightInstances {
    LightInstanceData instances[];
} lightBuffers[];

layout(push_constant) uniform PC {
    uint textureIdx;    // unused here; kept for layout parity across pipelines
    uint transformIdx;  // bindless slot of the light-instance SSBO
} pc;

layout(location = 0) in vec3 point;
layout(location = 1) in vec3 barycentric;
layout(location = 0) out vec3 pointOut;
layout(location = 1) out vec3 baryOut;
layout(location = 2) out flat int instanceIndex;

void main() {
    LightInstanceData inst = lightBuffers[nonuniformEXT(pc.transformIdx)].instances[gl_InstanceIndex];
    gl_Position   = globals.proj * globals.view * inst.model * vec4(point, 1.0);
    pointOut      = point;
    baryOut       = barycentric;
    instanceIndex = gl_InstanceIndex;
}
