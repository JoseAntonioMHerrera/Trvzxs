#version 450
#extension GL_EXT_nonuniform_qualifier : require

layout(set = 1, binding = 0) uniform FrameGlobals {
    mat4 view;
    mat4 proj;
    mat4 viewProj;
    mat4 invViewProj;
} globals;

layout(std430, set = 0, binding = 1) readonly buffer ModelTransforms {
    mat4 data[];
} transforms[];

layout(push_constant) uniform PC {
    uint textureIdx;   // unused here; kept for layout parity with model pass
    uint transformIdx;
} pc;

layout(location = 0) in vec3 inPosition;

void main() {
    mat4 model = transforms[nonuniformEXT(pc.transformIdx)].data[gl_InstanceIndex];
    gl_Position = globals.proj * globals.view * model * vec4(inPosition, 1.0);
}
