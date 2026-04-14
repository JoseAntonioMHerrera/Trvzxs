#version 450
#extension GL_EXT_nonuniform_qualifier : require

// Bindless — set 0 is the engine-wide descriptor table.
//   binding 0 : sampler2D textures[]     (fragment only)
//   binding 1 : buffer     storage[]     (typed views aliased per consumer)
//
// Per-pass — set 1 holds the model pass's frame-constant UBO.
// Push constants carry the per-draw bindless indices.

layout(set = 1, binding = 0) uniform FrameGlobals {
    mat4 view;
    mat4 proj;
    mat4 viewProj;
    mat4 invViewProj;
} globals;

layout(set = 0, binding = 1) readonly buffer ModelTransforms {
    mat4 data[];
} transforms[];

layout(push_constant) uniform PC {
    uint textureIdx;
    uint transformIdx;
} pc;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inNormal;

layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) out vec3 outNormal;

void main() {
    mat4 model = transforms[nonuniformEXT(pc.transformIdx)].data[gl_InstanceIndex];
    gl_Position  = globals.proj * globals.view * model * vec4(inPosition, 1.0);
    fragTexCoord = inTexCoord;
    outNormal    = normalize(mat3(model) * inNormal);
}
