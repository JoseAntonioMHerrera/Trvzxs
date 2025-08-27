#version 450
struct MVP {
    mat4 m;
    mat4 v;
    mat4 p;
};

struct CameraData{
    vec3 eye;
    vec3 center;
    vec3 up;
    vec2 screenSize;
};
struct LightInstanceData {
    vec4 color;
    vec4 position;
    float scale;
};
struct LightInstance {
    MVP mvp;
    mat4 inverseProjView;
    CameraData camera;
    LightInstanceData data;
};
layout(binding = 7) uniform UniformBufferObject {
LightInstance lightInstance[2];
} ubo;

layout(location = 0) in vec3 point;
layout(location = 1) in vec3 barycentric;
layout(location = 0) out vec3 pointOut;
layout(location = 1) out vec3 baryOut;
layout(location = 2) out flat int instanceIndex;

void main() {
    gl_Position = ubo.lightInstance[gl_InstanceIndex].mvp.p * ubo.lightInstance[gl_InstanceIndex].mvp.v * ubo.lightInstance[gl_InstanceIndex].mvp.m * vec4(point, 1.0);
    pointOut = point;
    baryOut = barycentric;
    instanceIndex = gl_InstanceIndex;
}
