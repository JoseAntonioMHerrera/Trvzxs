#version 450

// Per-frame globals (binding 9) — view/proj computed once per frame
layout(binding = 9) uniform FrameGlobals {
    mat4 view;
    mat4 proj;
    mat4 viewProj;
    mat4 invViewProj;
    // Camera3D: eye, center, up, screenSize (not needed in model shader)
} globals;

// Per-instance model matrices (binding 0) — one mat4 per live instance
layout(std430, binding = 0) buffer InstanceModels {
    mat4 model[];
} instances;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec4 inNormals;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec4 outNormals;

void main() {
    gl_Position = globals.proj * globals.view * instances.model[gl_InstanceIndex] * vec4(inPosition, 1.0);
    fragColor = inColor;
    fragTexCoord = inTexCoord;
    vec3 worldNormal = normalize(mat3(instances.model[gl_InstanceIndex]) * inNormals.xyz);
    outNormals = vec4(worldNormal, inNormals.w);
}
