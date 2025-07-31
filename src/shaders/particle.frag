#version 450

layout(location = 0) in vec4 fragColor;
layout(location = 0) out vec4 outColor;

void main() {
    // Apply gamma correction
    float gamma = 2.2;
    vec3 correctedColor = pow(fragColor.rgb, vec3(1.0 / gamma));

    // Set the output color
    outColor = vec4(1.0, 1.0, 0.0, 1.0);
}
