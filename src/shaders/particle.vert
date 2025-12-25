#version 450

layout(binding = 0) uniform UniformBufferObject {
   mat4 model; // TODO: model distinto por instancia, sacar view y proj
   mat4 view;
   mat4 proj;
} ubo;

// Define the particle structure
struct Particle {
   vec4 position;
   vec4 velocity;
   vec4 color;
};

layout(location = 0) in vec3 point;
layout(location = 1) in vec4 color;
layout(location = 2) in vec3 bary;

layout(std140, binding = 3) buffer ParticleBuffer {
   Particle particles[];
} particleBuffer;

layout(location = 0) out vec4 fragColor;

vec3 calculateBillboardPosition(vec4 particlePosition) {

    vec3 cameraRight = vec3(ubo.view[0][0], ubo.view[1][0], ubo.view[2][0]);
    vec3 cameraUp = vec3(ubo.view[0][1], ubo.view[1][1], ubo.view[2][1]);

    // Generate quad corner from vertex ID
    vec2 quadCorner = vec2(
        (gl_VertexIndex & 1) * 2.0 - 1.0,
        ((gl_VertexIndex >> 1) & 1) * 2.0 - 1.0
    );

    // Scale by billboard size
    vec2 offset = quadCorner * 0.2;

    // Transform to world space using extracted camera vectors
    vec3 worldPos = particlePosition.xyz +
                    offset.x * cameraRight +
                    offset.y * cameraUp;

    return worldPos;
}

void main() {
   gl_PointSize = 2.0;
   mat4 identity = mat4(1.0);
   vec3 worldPos =  calculateBillboardPosition(particleBuffer.particles[gl_InstanceIndex].position);
   gl_Position = ubo.proj * ubo.view * identity * vec4(worldPos, 1.0);
   fragColor = color;
}
