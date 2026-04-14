
#version 450

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec3 inNormal;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outNormals;
layout(binding = 1) uniform sampler2D texSampler;

void main() {
    vec4 texColor = texture(texSampler, fragTexCoord);

    float gamma = 2.2;
    vec3 correctedColor = pow(texColor.rgb, vec3(1.0 / gamma));

    outColor = vec4(correctedColor, gl_FragCoord.z);
    outNormals = vec4(inNormal, 0.0);
}
