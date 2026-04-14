#version 450

// Per-frame globals (binding 9) — view/proj computed once per frame
layout(binding = 9) uniform FrameGlobals {
    mat4 view;
    mat4 proj;
    mat4 viewProj;
    mat4 invViewProj;
} globals;

// Per-instance model matrices (binding 0) — one mat4 per live instance
layout(std430, binding = 0) buffer InstanceModels {
    mat4 model[];
} instances;

layout(location = 0) in vec3 inPosition;

void main() {
    gl_Position = globals.proj * globals.view * instances.model[gl_InstanceIndex] * vec4(inPosition, 1.0);
}
