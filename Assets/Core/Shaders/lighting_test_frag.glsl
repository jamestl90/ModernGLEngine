#version 460 core

in vec2 v_TexCoords;

layout(binding = 0) uniform sampler2D gAlbedoAO;
layout(binding = 1) uniform sampler2D gNormals;
layout(binding = 2) uniform sampler2D gMetallicRoughness;
layout(binding = 3) uniform sampler2D gEmissive;
layout(binding = 4) uniform sampler2D gDepth;
layout(binding = 5) uniform sampler2D gDLShadowMap;
layout(binding = 6) uniform samplerCube gSkyTexture;
layout(binding = 7) uniform samplerCube gIrradianceMap;
layout(binding = 8) uniform samplerCube gPrefilteredMap;
layout(binding = 9) uniform sampler2D gBRDFLUT;
layout(binding = 10) uniform sampler2D gPositions;

uniform mat4 u_LightSpaceMatrix; // Combined light projection and view matrix
uniform mat4 u_ViewInverse;
uniform mat4 u_ProjectionInverse;

uniform int u_PCFKernelSize;
uniform float u_Bias;

uniform float u_SpecularIndirectFactor;
uniform float u_DiffuseIndirectFactor;

uniform float u_Near;
uniform float u_Far;

uniform vec3 u_LightDirection;
uniform vec3 u_LightColor;      

uniform vec3 u_GridOrigin;
uniform vec3 u_ProbeSpacing;
uniform ivec3 u_GridResolution;

struct DDGIProbe
{
    vec4 WorldPosition;
    vec4 Irradiance;
    float HitDistance;
};

layout(std430, binding = 6) readonly buffer ProbeData 
{
    DDGIProbe perDrawData[];
};

layout(std140, binding = 4) uniform ShaderGlobalData 
{
    mat4 viewMatrix;
    mat4 projMatrix;
    vec4 camPos;
    vec4 camDir;
    vec2 timeInfo;
    vec2 windowSize;
    int frameCount;
};

layout(location = 0) out vec3 DirectLight;
layout(location = 1) out vec3 IBL;
layout(location = 2) out vec3 IndirectLight;

struct GBufferData 
{
    vec3 albedo;
    vec3 normal;
    vec3 worldPos;
    float ao;
    float metallic;
    float roughness;
    vec3 F0;
    vec3 emissive;
    float receiveShadows;
    float depth;
};

// Linearize depth from non-linear clip space
float LinearizeDepth(float depth)
{
    float z = depth * 2.0 - 1.0; // Convert to NDC space
    return (2.0 * u_Near * u_Far) / (u_Far + u_Near - z * (u_Far - u_Near));
}

vec3 ReconstructWorldPosFromDepth(vec2 texCoords, float depth)
{
    float z = depth * 2.0 - 1.0; // NDC depth

    vec4 clipSpacePos = vec4(texCoords * 2.0 - 1.0, z, 1.0);
    vec4 viewSpacePos = u_ProjectionInverse * clipSpacePos;
    viewSpacePos /= viewSpacePos.w;

    vec4 worldSpacePos = u_ViewInverse * viewSpacePos;
    return worldSpacePos.xyz;
}

// From non-linear depth buffer directly to view-space position
vec3 ReconstructViewPosFromDepth(vec2 texCoords, float depth)
{
    float z = depth * 2.0 - 1.0; // Convert depth to NDC [-1,1]
    
    vec4 clipSpacePos = vec4(texCoords * 2.0 - 1.0, z, 1.0);
    vec4 viewSpacePos = u_ProjectionInverse * clipSpacePos;
    viewSpacePos /= viewSpacePos.w;
    
    return viewSpacePos.xyz;
}

// Shadow calculation function
float ShadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir) 
{
   // Perform perspective divide to get normalized device coordinates
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;

    // Transform to [0, 1] range for shadow map sampling
    projCoords = projCoords * 0.5 + 0.5;

    if (projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0) 
    {
        return 1.0; // Outside the shadow map, fully lit
    }

    float closestDepth = texture(gDLShadowMap, projCoords.xy).r;
    float currentDepth = projCoords.z;
    float bias = max(u_Bias * tan(acos(dot(normal, lightDir))), 0.0001);
    float shadow = (currentDepth - bias) > closestDepth ? 0.0 : 1.0;

    // Optional: Percentage Closer Filtering (PCF)
    if (u_PCFKernelSize != 0)
    {
        float pcfShadow = 0.0;
        float texelSize = 1.0 / 4096.0;
        for (int x = -u_PCFKernelSize; x <= u_PCFKernelSize; ++x) 
        {
            for (int y = -u_PCFKernelSize; y <= u_PCFKernelSize; ++y) 
            {
                vec2 offset = vec2(x, y) * texelSize;
                float pcfDepth = texture(gDLShadowMap, projCoords.xy + offset).r;
                pcfShadow += (currentDepth - bias) > pcfDepth ? 0.0 : 1.0;
            }
        }
        shadow = pcfShadow / float((2 * u_PCFKernelSize + 1) * (2 * u_PCFKernelSize + 1));
    }
    return shadow;
}

// Fresnel-Schlick approximation for specular reflection
vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness) 
{
    // return F0 + (vec3(1.0) - F0) * pow(1.0 - cosTheta, 5.0);
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}

// GGX Normal Distribution Function
float ggxNDF(float NdotH, float roughness) 
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH2 = NdotH * NdotH;

    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    return a2 / (3.14159265359 * denom * denom);
}

// Smith geometry term
float geometrySmith(float NdotV, float NdotL, float roughness) 
{
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    float gV = NdotV / (NdotV * (1.0 - k) + k);
    float gL = NdotL / (NdotL * (1.0 - k) + k);
    return gV * gL;
}

vec3 CalculateDirectLighting(GBufferData gData, float shadow, vec3 lightDirVS, vec3 viewDirVS)
{
    vec3 normalVS = gData.normal; // already in view space
    vec3 halfDir = normalize(lightDirVS + viewDirVS);

    float NdotL = max(dot(normalVS, lightDirVS), 0.0);
    float NdotV = max(dot(normalVS, viewDirVS), 0.0);
    float NdotH = max(dot(normalVS, halfDir), 0.0);
    float VdotH = max(dot(viewDirVS, halfDir), 0.0);

    vec3 F = fresnelSchlickRoughness(VdotH, gData.F0, gData.roughness);
    float NDF = ggxNDF(NdotH, gData.roughness);
    float G = geometrySmith(NdotV, NdotL, gData.roughness);

    vec3 specular = F * NDF * G / max(4.0 * NdotV * NdotL, 0.001);
    vec3 diffuse = (1.0 - F) * gData.albedo / 3.14159265359;

    return (diffuse + specular) * u_LightColor * NdotL * shadow;
}

// Diffuse IBL calculation
vec3 CalculateDiffuseIBL(vec3 normal, vec3 albedo)
{
    vec3 irradiance = texture(gIrradianceMap, normal).rgb;
    return irradiance * albedo * u_DiffuseIndirectFactor;
}

// Specular IBL calculation
vec3 CalculateSpecularIBL(vec3 normal, vec3 viewDir, float roughness, vec3 F0)
{
    vec3 reflection = reflect(-viewDir, normal);
    vec3 prefilteredColor = textureLod(gPrefilteredMap, reflection, roughness * 4.0).rgb;
    float NdotV = max(dot(normal, viewDir), 0.0);
    vec2 brdf = texture(gBRDFLUT, vec2(NdotV, roughness)).rg;

    return prefilteredColor * (F0 * brdf.x + brdf.y) * u_SpecularIndirectFactor;
}

GBufferData ExtractGBufferData(vec2 texCoords)
{
    GBufferData gData;

    vec4 normalSample = texture(gNormals, texCoords);
    vec4 albedoAOSample = texture(gAlbedoAO, texCoords);
    gData.albedo = albedoAOSample.rgb;
    gData.ao = max(albedoAOSample.a, 0.0);
    gData.normal = normalize(normalSample.xyz);
    gData.depth = texture(gDepth, texCoords).r;
    gData.worldPos = ReconstructWorldPosFromDepth(texCoords, gData.depth);
    vec4 metallicRoughness = texture(gMetallicRoughness, texCoords);
    gData.metallic = metallicRoughness.b;
    gData.roughness = max(metallicRoughness.g, 0.05);
    gData.F0 = mix(vec3(0.04), gData.albedo, gData.metallic);
    gData.emissive = texture(gEmissive, texCoords).rgb;
    gData.receiveShadows = normalSample.w;

    return gData;
}

vec3 ApplyToneMapping(vec3 color)
{
    return color / (color + vec3(1.0));
}

vec3 ApplyGammaCorrection(vec3 color)
{
    return pow(color, vec3(1.0 / 2.2));
}

// DDGI Functions
int Flatten3DIndex(ivec3 coord, ivec3 resolution) 
{
    return coord.x + coord.y * resolution.x + coord.z * resolution.x * resolution.y;
}

bool IsInBounds(ivec3 coord, ivec3 resolution) 
{
    return all(greaterThanEqual(coord, ivec3(0))) && all(lessThan(coord, resolution));
}

vec3 SampleDDGI(vec3 worldPos, vec3 normalWS)
{
    vec3 gridCoord = (worldPos - u_GridOrigin) / u_ProbeSpacing;
    ivec3 baseCoord = ivec3(floor(gridCoord));
    vec3 localCoord = fract(gridCoord);

    vec3 irradiance = vec3(0.0);
    float totalWeight = 0.0;

    for (int z = 0; z <= 1; ++z)
    for (int y = 0; y <= 1; ++y)
    for (int x = 0; x <= 1; ++x)
    {
        ivec3 coord = baseCoord + ivec3(x, y, z);
        if (!IsInBounds(coord, ivec3(u_GridResolution))) continue;

        int index = Flatten3DIndex(coord, ivec3(u_GridResolution));
        DDGIProbe probe = perDrawData[index];

        vec3 probePos = probe.WorldPosition.xyz;
        vec3 toProbe = worldPos - probePos;
        float dist = length(toProbe);

        float visibilityWeight = clamp(1.0 - dist / max(probe.HitDistance, 0.001), 0.0, 1.0);
        float normalWeight = max(dot(normalWS, normalize(toProbe)), 0.0);
        float weight = visibilityWeight * normalWeight + 0.0001;

        irradiance += probe.Irradiance.rgb * weight;
        totalWeight += weight;
    }

    return irradiance / max(totalWeight, 0.0001);
}

// main
void main() 
{
    GBufferData gData = ExtractGBufferData(v_TexCoords);

    //float linearDepth = LinearizeDepth(gData.depth);
    vec3 viewPos = ReconstructViewPosFromDepth(v_TexCoords, gData.depth);
    vec3 viewDir = normalize(-viewPos);
    vec3 viewDirWS = normalize(camPos.xyz - gData.worldPos);
    vec3 lightDirWS = normalize(-u_LightDirection);
    vec3 lightDirView = normalize((viewMatrix * vec4(-u_LightDirection, 0.0)).xyz);

    if (gData.depth > 0.999)
    {
        DirectLight = texture(gSkyTexture, -viewDirWS).rgb;
        return;
    }

    vec3 normalWS = normalize((u_ViewInverse * vec4(gData.normal, 0.0)).xyz);
    vec4 fragPosLightSpace = u_LightSpaceMatrix * vec4(gData.worldPos, 1.0);
    float shadow = 1.0;
    if (gData.receiveShadows > 0.0)
    {
        shadow = ShadowCalculation(fragPosLightSpace, normalWS, lightDirWS);
    }

    // direct contribution
    DirectLight = CalculateDirectLighting(gData, shadow, lightDirView, viewDir);

    // hdr sky contributions 
    vec3 iblDiffuse = CalculateDiffuseIBL(normalWS, gData.albedo);
    vec3 iblSpecular = CalculateSpecularIBL(normalWS, viewDirWS, gData.roughness, gData.F0);
    IBL = iblDiffuse + iblSpecular;

    // global illumination contribution
    IndirectLight = SampleDDGI(gData.worldPos, normalWS);

    //vec3 totalLighting = lighting + iblDiffuse + iblSpecular;
    //totalLighting += gData.emissive;
    //totalLighting *= gData.ao;

    ////totalLighting = ApplyToneMapping(totalLighting);
    ////totalLighting = ApplyGammaCorrection(totalLighting);

    //FragColor = vec4(gData.raymarchGI, 1.0);
}
