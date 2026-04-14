#version 450
#extension GL_EXT_nonuniform_qualifier : require

layout(set = 0, binding = 0) uniform sampler2D textures[];

layout(push_constant) uniform PC {
    uint textureIdx;
    uint transformIdx;
} pc;

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec3 inNormal;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outNormals;

void main() {
    vec4 texColor = texture(textures[nonuniformEXT(pc.textureIdx)], fragTexCoord);

    float gamma = 2.2;
    vec3 correctedColor = pow(texColor.rgb, vec3(1.0 / gamma));

    outColor   = vec4(correctedColor, gl_FragCoord.z);
    outNormals = vec4(inNormal, 0.0);
}
