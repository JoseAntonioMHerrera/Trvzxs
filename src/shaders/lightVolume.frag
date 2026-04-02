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
    LightInstance lightInstance[2000]; // Migrar a SSBO buffers
} ubo;

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
    vec4 lightInput = ubo.lightInstance[instanceIndex].data.position;
    vec3 lightPos = lightInput.xyz;
    uint index = uint(gl_FragCoord.y) * uint(ubo.lightInstance[instanceIndex].camera.screenSize.x) + uint(gl_FragCoord.x);
    /*pixels[index].pixel.r = lightPos.x;
    pixels[index].pixel.g = lightPos.y;
    pixels[index].pixel.b = lightPos.z;
    pixels[index].pixel.a = 0.0;
    pixels[index].pixel2.r = normal.x;
    pixels[index].pixel2.g = normal.y;
    pixels[index].pixel2.b = normal.z;
    pixels[index].pixel3.a = 0.0;
    pixels[index].pixel3.r = albedo.x;
    pixels[index].pixel3.g = albedo.y;
    pixels[index].pixel3.b = albedo.z;
    pixels[index].pixel3.a = 0.0;*/
    vec3 lightColor = ubo.lightInstance[instanceIndex].data.color.xyz;  // Define lightColor
    float lightIntensity = 1.0;
    float specularPower = 32.0; // Shininess

    vec3 lightDir = normalize(lightPos - worldPos);
    vec3 viewDir = normalize(ubo.lightInstance[instanceIndex].camera.eye.xyz - worldPos);  // Camera to surface
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
    vec2 screenUV = gl_FragCoord.xy / ubo.lightInstance[instanceIndex].camera.screenSize;
    vec4 geoData = texture(geometryBuffer, screenUV);
    vec4 normalData = texture(normalsBuffer, screenUV);

     // Calculate distance to nearest edge
    float edgeDistance = min(min(baryIn.x, baryIn.y), baryIn.z);

    // Define wireframe thickness
    float wireframe = smoothstep(0.0, 0.02, edgeDistance);

    // Discard fragments not on edges (creates pure wireframe)
    float geometryDepth = geoData.a;


    vec4 lightInput = ubo.lightInstance[instanceIndex].data.position;
    vec3 lightPos = lightInput.xyz;

    // Project light position to screen space
    vec4 lightClipPos = ubo.lightInstance[instanceIndex].mvp.p *
                        ubo.lightInstance[instanceIndex].mvp.v *
                        vec4(lightPos, 1.0);
    vec3 lightNDC = lightClipPos.xyz / lightClipPos.w;
    vec2 lightScreenPos = (lightNDC.xy * 0.5 + 0.5) * ubo.lightInstance[instanceIndex].camera.screenSize;

    // Check if current fragment is close to projected light position
    float screenDistance = distance(gl_FragCoord.xy, lightScreenPos);
    if (screenDistance < 20.0) {  // 5 pixel radius
        outColor = vec4(ubo.lightInstance[instanceIndex].data.color.xyz, geoData.a);  // dot
        return;
    }



    vec3 albedo = geoData.rgb;
    if (wireframe > 0.5) {
            if(gl_FragCoord.z < geoData.a){
                discard;
            }else {
                vec2 screenCoord = gl_FragCoord.xy / ubo.lightInstance[instanceIndex].camera.screenSize;
                vec4 clipSpacePos = vec4(screenCoord * 2.0 - 1.0, geometryDepth, 1.0);
                vec4 worldPos = ubo.lightInstance[instanceIndex].inverseProjView * clipSpacePos;
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
                vec2 screenCoord = gl_FragCoord.xy / ubo.lightInstance[instanceIndex].camera.screenSize;
                vec4 clipSpacePos = vec4(screenCoord * 2.0 - 1.0, geometryDepth, 1.0);
                vec4 worldPos = ubo.lightInstance[instanceIndex].inverseProjView * clipSpacePos;
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
