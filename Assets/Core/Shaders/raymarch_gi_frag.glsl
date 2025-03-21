#version 460

// Output
out vec3 MetallicReflection;

// Texture bindings
layout(binding = 0) uniform sampler2D albedoTex;
layout(binding = 1) uniform sampler2D normalTex;
layout(binding = 2) uniform sampler2D metallicRoughnessTex;
layout(binding = 3) uniform sampler2D depthTex;
layout(binding = 4) uniform sampler2D previousFrameIndirect;
layout(binding = 5) uniform sampler2D directLightTex;
layout(binding = 6) uniform samplerCube gSkyTexture;

layout(std140, binding = 0) uniform ShaderGlobalData 
{
    mat4 viewMatrix;
    mat4 projMatrix;
    vec4 camPos;
    vec4 camDir;
    vec2 timeInfo;
    vec2 windowSize;
    int frameCount;
};

uniform float u_Near;
uniform float u_Far;

in vec2 v_TexCoords;

const int MAX_STEPS = 500;
const float STEP_SIZE = 0.05;
const float HIT_THRESHOLD = 0.02;

struct RayMarchResult {
    vec3 color;
    vec3 scenePos;
    float depthDiff;
    vec2 hitUV;
    bool hit;
};

// Reconstruct full view-space position from screen UV + depth
vec3 ReconstructViewPosFromDepth(vec2 texCoords, float depth, mat4 inverseProj) {
    float z = depth * 2.0 - 1.0;
    vec4 clip = vec4(texCoords * 2.0 - 1.0, z, 1.0);
    vec4 view = inverseProj * clip;
    return view.xyz / view.w;
}

// Reconstruct only Z (view space) from depth buffer
float ReconstructViewZ(float depth, mat4 inverseProj) {
    float z = depth * 2.0 - 1.0;
    vec4 clip = vec4(0.0, 0.0, z, 1.0);
    vec4 view = inverseProj * clip;
    return view.z / view.w;
}

RayMarchResult RayMarch(vec3 startPosVS, vec3 rayDirVS, mat4 inverseProj, mat4 viewInv)
{
    RayMarchResult result;
    result.color = vec3(0.0);
    result.scenePos = vec3(0.0);
    result.depthDiff = 1.0;
    result.hitUV = vec2(0.0);
    result.hit = false;

    vec3 currentPosVS = startPosVS + rayDirVS * STEP_SIZE;

    for (int i = 0; i < MAX_STEPS; ++i)
    {
        // Bail if we're behind the near plane
        if (currentPosVS.z > -u_Near) break;

        // Project to NDC
        vec4 clipPos = projMatrix * vec4(currentPosVS, 1.0);
        if (abs(clipPos.w) < 1e-4) break;

        vec3 ndc = clipPos.xyz / clipPos.w;
        vec2 screenUV = ndc.xy * 0.5 + 0.5;

        // Exit if outside screen
        if (any(lessThan(screenUV, vec2(0.0))) || any(greaterThan(screenUV, vec2(1.0))))
            break;

        float sceneDepth = texture(depthTex, screenUV).r;

        //if (sceneDepth <= 0.001 || sceneDepth >= 0.999)
        //    break;

        float sceneViewZ = ReconstructViewZ(sceneDepth, inverseProj);
        float depthDiff = abs(currentPosVS.z - sceneViewZ);

        // Fix false positives: ray must be in front of scene surface
        bool isInFront = currentPosVS.z < sceneViewZ;

        if (isInFront && depthDiff < HIT_THRESHOLD * max(abs(sceneViewZ), 0.01))
        {
            result.hit = true;
            result.hitUV = screenUV;
            result.color = texture(albedoTex, screenUV).rgb;
            result.scenePos = currentPosVS;
            result.depthDiff = depthDiff;
            return result;
        }

        currentPosVS += rayDirVS * STEP_SIZE;
    }

    // Fallback color if miss
    vec3 worldDir = normalize((viewInv * vec4(rayDirVS, 0.0)).xyz);
    vec3 skyColor = texture(gSkyTexture, worldDir).rgb;

    result.color = mix(texture(previousFrameIndirect, v_TexCoords).rgb, skyColor, 0.5);
    return result;
}

void main()
{
    mat4 inverseProj = inverse(projMatrix);
    mat4 inverseView = inverse(viewMatrix);

    vec3 fragNormalVS = normalize(texture(normalTex, v_TexCoords).xyz);
    float depth = texture(depthTex, v_TexCoords).r;

    vec3 fragViewPos = ReconstructViewPosFromDepth(v_TexCoords, depth, inverseProj);
    vec3 viewDir = normalize(-fragViewPos);
    vec3 rayDirVS = reflect(-viewDir, fragNormalVS);

    float metallic = texture(metallicRoughnessTex, v_TexCoords).b;
    if (metallic <= 0.001) {
        MetallicReflection = vec3(0.0);
        return;
    }

    RayMarchResult result = RayMarch(fragViewPos, rayDirVS, inverseProj, inverseView);

    // Optional: fade based on screen edges or view angle
    float screenFade = smoothstep(0.8, 1.0, length(v_TexCoords - 0.5) * 1.5);
    MetallicReflection = mix(result.color, vec3(0.0), screenFade);
}
