#version 450

layout(binding = 0) uniform UniformBufferObject {
   mat4 model;
   mat4 view;
   mat4 proj;
} ubo;

// Define the particle structure
struct Particle {
   vec3 position;
   vec3 velocity;
   vec4 color;
};

layout(std140, binding = 3) buffer ParticleBuffer {
   Particle particles[];
} particleBuffer;

layout(location = 0) out vec4 fragColor;

void main() {
   uint particleIndex = gl_VertexIndex;
   gl_PointSize = 2.0;
   gl_Position = ubo.proj * ubo.view * ubo.model * vec4(particleBuffer.particles[particleIndex].position, 1.0);
   fragColor = particleBuffer.particles[particleIndex].color;
}
