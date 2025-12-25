#version 450

struct InstanceData {
    mat4 model;
    mat4 view;
    mat4 proj;
};

layout(binding = 0) uniform UniformBufferObject {
    InstanceData instances[2000];
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec4 inNormals;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec4 outNormals;

void main() {
    gl_Position = ubo.instances[gl_InstanceIndex].proj * ubo.instances[gl_InstanceIndex].view * ubo.instances[gl_InstanceIndex].model * vec4(inPosition, 1.0);
    fragColor = inColor;
    fragTexCoord = inTexCoord;
    vec3 worldNormal = normalize(mat3(ubo.instances[gl_InstanceIndex].model) * inNormals.xyz);
    outNormals = vec4(worldNormal, inNormals.w);
}
