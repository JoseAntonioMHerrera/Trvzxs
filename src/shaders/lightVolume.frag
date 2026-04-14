#version 450
#extension GL_EXT_nonuniform_qualifier : require

layout(set = 1, binding = 0) uniform FrameGlobals {
    mat4 view;
    mat4 proj;
    mat4 viewProj;
    mat4 invViewProj;
    vec3 cameraEye;
    float _pad1;
    vec3 cameraCenter;
    float _pad2;
    vec3 cameraUp;
    float _pad3;
    vec2 screenSize;
    vec2 _pad4;
} globals;

struct LightInstanceData {
    mat4 model;
    vec4 color;
    vec3 position;
    float scale;
};

layout(std430, set = 0, binding = 1) readonly buffer LightInstances {
    LightInstanceData instances[];
} lightBuffers[];

layout(set = 1, binding = 1) uniform sampler2D geometryBuffer;
layout(set = 1, binding = 2) uniform sampler2D normalsBuffer;

layout(push_constant) uniform PC {
    uint textureIdx;
    uint transformIdx;
} pc;

layout(location = 0) in vec3 point;
layout(location = 1) in vec3 baryIn;
layout(location = 2) in flat int instanceIndex;

layout(location = 0) out vec4 outColor;

vec3 calculateBlinnPhong(vec3 worldPos, vec3 normal, vec3 albedo) {
    LightInstanceData inst = lightBuffers[nonuniformEXT(pc.transformIdx)].instances[instanceIndex];
    vec3  lightPos       = inst.position;
    vec3  lightColor     = inst.color.xyz;
    float lightIntensity = 1.0;
    float specularPower  = 32.0;

    vec3 lightDir   = normalize(lightPos - worldPos);
    vec3 viewDir    = normalize(globals.cameraEye - worldPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);

    float lightHalfExtent = inst.scale * 5.0;
    float distance        = length(lightPos - worldPos) * (30.0 / lightHalfExtent);
    float attenuation     = 1.0 / (1.0 + 0.09 * distance + 0.032 * distance * distance);

    vec3 ambient  = 0.7 * albedo;
    float diff    = max(dot(normal, lightDir), 0.0);
    vec3 diffuse  = diff * albedo * lightColor;
    float spec    = pow(max(dot(normal, halfwayDir), 0.0), specularPower);
    vec3 specular = spec * lightColor * 0.5;

    return (ambient + diffuse + specular) * attenuation * lightIntensity;
}

void main() {
    vec2 screenUV    = gl_FragCoord.xy / globals.screenSize;
    vec4 geoData     = texture(geometryBuffer, screenUV);
    vec4 normalData  = texture(normalsBuffer, screenUV);

    float edgeDistance = min(min(baryIn.x, baryIn.y), baryIn.z);
    float wireframe    = smoothstep(0.0, 0.02, edgeDistance);
    float geometryDepth = geoData.a;

    LightInstanceData inst = lightBuffers[nonuniformEXT(pc.transformIdx)].instances[instanceIndex];

    vec4 lightClipPos    = globals.proj * globals.view * vec4(inst.position, 1.0);
    vec3 lightNDC        = lightClipPos.xyz / lightClipPos.w;
    vec2 lightScreenPos  = (lightNDC.xy * 0.5 + 0.5) * globals.screenSize;
    float screenDistance = distance(gl_FragCoord.xy, lightScreenPos);
    if (screenDistance < 20.0) {
        outColor = vec4(inst.color.xyz, geoData.a);
        return;
    }

    vec3 albedo = geoData.rgb;
    if (wireframe > 0.5) {
        if (gl_FragCoord.z < geoData.a) {
            discard;
        } else {
            vec4 clipSpacePos = vec4(screenUV * 2.0 - 1.0, geometryDepth, 1.0);
            vec4 worldPos     = globals.invViewProj * clipSpacePos;
            worldPos         /= worldPos.w;
            vec3 normal       = normalize(normalData.xyz);
            vec3 lighting     = calculateBlinnPhong(worldPos.xyz, normal, albedo);
            outColor          = vec4(lighting, geoData.a);
        }
    } else {
        if (albedo.r != 0.0 && albedo.g != 0.0 && albedo.b != 0.0) {
            if (gl_FragCoord.z < geoData.a) {
                outColor = vec4(1.0, 1.0, 1.0, 1.0);
            } else {
                vec4 clipSpacePos = vec4(screenUV * 2.0 - 1.0, geometryDepth, 1.0);
                vec4 worldPos     = globals.invViewProj * clipSpacePos;
                worldPos         /= worldPos.w;
                vec3 normal       = normalize(normalData.xyz);
                vec3 lighting     = calculateBlinnPhong(worldPos.xyz, normal, albedo);
                outColor          = vec4(lighting, geoData.a);
            }
        } else {
            outColor = vec4(1.0, 1.0, 1.0, 1.0);
        }
    }
}
