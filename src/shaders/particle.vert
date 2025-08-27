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

layout(location = 0) in vec3 point;
layout(location = 1) in vec4 color;
layout(location = 2) in vec3 bary;

layout(std140, binding = 3) buffer ParticleBuffer {
   Particle particles[];
} particleBuffer;

layout(location = 0) out vec4 fragColor;

void main() {
   uint particleIndex = gl_VertexIndex;
   gl_PointSize = 2.0;
   gl_Position = ubo.proj * ubo.view * ubo.model * vec4(point, 1.0);
   fragColor = color;
}
