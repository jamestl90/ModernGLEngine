#version 450

layout(location = 0) out vec3 IndirectDiffuse;

layout(binding = 0) uniform sampler2D depthTex;
layout(binding = 1) uniform sampler2D normalTex;
layout(binding = 2) uniform sampler2D albedoTex;

uniform mat4 invProjection; // Inverse Projection Matrix
uniform mat4 viewMatrix;    // View Matrix
uniform vec2 screenSize;
uniform float maxRayDistance = 10.0; // Maximum distance for ray marching
uniform int numSteps = 16; // Number of steps for ray marching

in vec2 TexCoords;

vec3 ReconstructWorldPosition(vec2 uv, float depth) {
    vec4 clipSpace = vec4(uv * 2.0 - 1.0, depth, 1.0);
    vec4 worldSpace = invProjection * clipSpace;
    return worldSpace.xyz / worldSpace.w;
}

vec3 ScreenSpaceRayMarch(vec3 origin, vec3 direction) {
    float stepSize = maxRayDistance / float(numSteps);
    vec3 hitColor = vec3(0.0);
    
    for (int i = 1; i <= numSteps; i++) {
        vec3 samplePos = origin + direction * (stepSize * i);
        vec4 viewPos = viewMatrix * vec4(samplePos, 1.0);
        vec2 uv = (viewPos.xy / viewPos.w) * 0.5 + 0.5;

        if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) break;

        float sampleDepth = texture(depthTex, uv).r;
        vec3 sampleWorldPos = ReconstructWorldPosition(uv, sampleDepth);

        if (length(sampleWorldPos - origin) < stepSize * 2.0) {
            hitColor += texture(albedoTex, uv).rgb * 0.1;
        }
    }

    return hitColor;
}

void main() {
    float depth = texture(depthTex, TexCoords).r;
    vec3 worldPos = ReconstructWorldPosition(TexCoords, depth);
    vec3 normal = texture(normalTex, TexCoords).rgb * 2.0 - 1.0;

    // Generate a random diffuse direction
    vec3 randomDir = normalize(vec3(
        fract(sin(dot(worldPos.xy, vec2(12.9898, 78.233))) * 43758.5453),
        fract(sin(dot(worldPos.yz, vec2(12.9898, 78.233))) * 43758.5453),
        1.0
    ));

    // Ensure the direction is facing outward
    if (dot(randomDir, normal) < 0.0) {
        randomDir = -randomDir;
    }

    vec3 indirectLighting = ScreenSpaceRayMarch(worldPos, randomDir);

    IndirectDiffuse = indirectLighting;
}
