#version 450

layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 outColor;

layout(input_attachment_index = 0, binding = 5) uniform subpassInput geometryBuffer;
layout(input_attachment_index = 1, binding = 6) uniform subpassInput normalsBuffer;

void main() {
    // Read from G-buffer (we'll just verify it works)
    vec4 geoData = subpassLoad(geometryBuffer);
    vec4 normalData = subpassLoad(normalsBuffer);

    // Output fixed red color for testing
    outColor = vec4(1.0, 0.0, 0.0, 1.0);
}
