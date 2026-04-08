#version 450

// Per-frame globals (binding 9) — shared camera/projection data
layout(binding = 9) uniform FrameGlobals {
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

layout(std430, binding = 7) buffer LightInstances {
    LightInstanceData instances[];
} lights;

layout(binding = 5) uniform sampler2D geometryBuffer;
layout(binding = 6) uniform sampler2D normalsBuffer;

layout(location = 0) in vec3 point;
layout(location = 1) in vec3 baryIn;
layout(location = 2) in flat int instanceIndex;

/*

struct PixelData {
    Color pixel;
    Color pixel2;
    Color pixel3;
    Color pixel4;
};

struct Color {
    float r;
    float g;
    float b;
    float a;
};

layout(std140, binding = 8) buffer LightPixelStorageBuffer {
    PixelData pixels[];
};

*/

layout(location = 0) out vec4 outColor;  // writes to colorAttachmentRef

vec3 calculateBlinnPhong(vec3 worldPos, vec3 normal, vec3 albedo) {
    // Hardcoded light properties for testing
    vec3 lightPos = lights.instances[instanceIndex].position;
    uint index = uint(gl_FragCoord.y) * uint(globals.screenSize.x) + uint(gl_FragCoord.x);
    vec3 lightColor = lights.instances[instanceIndex].color.xyz;
    float lightIntensity = 1.0;
    float specularPower = 32.0; // Shininess

    vec3 lightDir = normalize(lightPos - worldPos);
    vec3 viewDir = normalize(globals.cameraEye - worldPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);

    float distance = length(lightPos - worldPos);
    float attenuation = 1.0 / (1.0 + 0.09 * distance + 0.032 * distance * distance);

    // Ambient (simulated global illumination)
    vec3 ambient = 0.7 * albedo;

    // Diffuse
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * albedo * lightColor;

    // Specular (Blinn-Phong)
    float spec = pow(max(dot(normal, halfwayDir), 0.0), specularPower);
    vec3 specular = spec * lightColor * 0.5; // Scale specular contribution

    return (ambient + diffuse + specular) * attenuation * lightIntensity;
}

void main() {
    // Read from G-buffer via screen-space UV
    vec2 screenUV = gl_FragCoord.xy / globals.screenSize;
    vec4 geoData = texture(geometryBuffer, screenUV);
    vec4 normalData = texture(normalsBuffer, screenUV);

     // Calculate distance to nearest edge
    float edgeDistance = min(min(baryIn.x, baryIn.y), baryIn.z);

    // Define wireframe thickness
    float wireframe = smoothstep(0.0, 0.02, edgeDistance);

    // Discard fragments not on edges (creates pure wireframe)
    float geometryDepth = geoData.a;


    vec3 lightPos = lights.instances[instanceIndex].position;

    // Project light position to screen space
    vec4 lightClipPos = globals.proj * globals.view * vec4(lightPos, 1.0);
    vec3 lightNDC = lightClipPos.xyz / lightClipPos.w;
    vec2 lightScreenPos = (lightNDC.xy * 0.5 + 0.5) * globals.screenSize;

    // Check if current fragment is close to projected light position
    float screenDistance = distance(gl_FragCoord.xy, lightScreenPos);
    if (screenDistance < 20.0) {  // 5 pixel radius
        outColor = vec4(lights.instances[instanceIndex].color.xyz, geoData.a);  // dot
        return;
    }



    vec3 albedo = geoData.rgb;
    if (wireframe > 0.5) {
            if(gl_FragCoord.z < geoData.a){
                discard;
            }else {
                vec2 screenCoord = gl_FragCoord.xy / globals.screenSize;
                vec4 clipSpacePos = vec4(screenCoord * 2.0 - 1.0, geometryDepth, 1.0);
                vec4 worldPos = globals.invViewProj * clipSpacePos;
                worldPos /= worldPos.w;
                vec3 worldPosition = worldPos.xyz;

                vec3 normal = normalize(normalData.xyz);
                vec3 lighting = calculateBlinnPhong(worldPosition, normal, albedo);
                outColor = vec4(lighting, geoData.a); // LIght
            }


    }else{
        // Color the wireframe
        if(albedo.r != 0.0 && albedo.g != 0.0 && albedo.b != 0.0) {
            if(gl_FragCoord.z < geoData.a){
                outColor = vec4(1.0, 1.0, 1.0, 1.0); // White wireframe
            }else{
                vec2 screenCoord = gl_FragCoord.xy / globals.screenSize;
                vec4 clipSpacePos = vec4(screenCoord * 2.0 - 1.0, geometryDepth, 1.0);
                vec4 worldPos = globals.invViewProj * clipSpacePos;
                worldPos /= worldPos.w;
                vec3 worldPosition = worldPos.xyz;
                vec3 normal = normalize(normalData.xyz);
                vec3 lighting = calculateBlinnPhong(worldPosition, normal, albedo);
                outColor = vec4(lighting, geoData.a); // White wireframe
            }
        }else{
            outColor = vec4(1.0, 1.0, 1.0, 1.0); // White wireframe
        }


    }
}
