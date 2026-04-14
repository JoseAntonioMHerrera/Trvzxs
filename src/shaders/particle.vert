#version 450
#extension GL_EXT_nonuniform_qualifier : require

layout(set = 1, binding = 0) uniform FrameGlobals {
    mat4 view;
    mat4 proj;
    mat4 viewProj;
    mat4 invViewProj;
} globals;

struct Particle {
   vec4 position;
   vec4 velocity;
   vec4 color;
};

layout(location = 0) in vec3 point;
layout(location = 1) in vec4 color;
layout(location = 2) in vec3 bary;

layout(std140, set = 0, binding = 1) readonly buffer ParticleBuffer {
   Particle data[];
} particleBuffers[];

layout(push_constant) uniform PC {
    uint textureIdx;    // unused; retained for engine-shared PC layout
    uint transformIdx;  // bindless slot of the current-frame particle SSBO
} pc;

layout(location = 0) out vec4 fragColor;

vec3 calculateBillboardPosition(vec4 particlePosition) {
    vec3 cameraRight = vec3(globals.view[0][0], globals.view[1][0], globals.view[2][0]);
    vec3 cameraUp    = vec3(globals.view[0][1], globals.view[1][1], globals.view[2][1]);
    vec2 quadCorner  = vec2(
        (gl_VertexIndex & 1) * 2.0 - 1.0,
        ((gl_VertexIndex >> 1) & 1) * 2.0 - 1.0
    );
    vec2 offset = quadCorner * 0.2;
    return particlePosition.xyz + offset.x * cameraRight + offset.y * cameraUp;
}

void main() {
    gl_PointSize = 2.0;
    vec4 pPos    = particleBuffers[nonuniformEXT(pc.transformIdx)].data[gl_InstanceIndex].position;
    vec3 wp      = calculateBillboardPosition(pPos);
    gl_Position  = globals.proj * globals.view * vec4(wp, 1.0);
    fragColor    = color;
}
