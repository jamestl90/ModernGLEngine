#version 460 core

in vec2 v_TexCoords;

layout(binding = 0) uniform sampler2D gAlbedoAO;
layout(binding = 1) uniform sampler2D gNormals;
layout(binding = 2) uniform sampler2D gMetallicRoughness;
layout(binding = 3) uniform sampler2D gEmissive;
layout(binding = 4) uniform sampler2D gDepth;
layout(binding = 5) uniform sampler2D gDLShadowMap;
layout(binding = 5) uniform samplerCube gSkyTexture;
layout(binding = 7) uniform samplerCube gIrradianceMap;
layout(binding = 8) uniform samplerCube gPrefilteredMap;
layout(binding = 9) uniform sampler2D gBRDFLUT;

uniform mat4 u_LightSpaceMatrix; // Combined light projection and view matrix
uniform mat4 u_ViewInverse;
uniform mat4 u_ProjectionInverse;

uniform int u_PCFKernelSize;
uniform float u_Bias;

uniform float u_Near;
uniform float u_Far;

uniform vec3 u_LightDirection; // Directional light direction (normalized)
uniform vec3 u_LightColor;     // Directional light color
uniform vec3 u_CameraPos;      // Camera position for view direction

out vec4 FragColor;

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
};

// Linearize depth from non-linear clip space
float LinearizeDepth(float depth, float near, float far) 
{
    float z = depth * 2.0 - 1.0; // Convert to NDC space
    return (2.0 * near * far) / (far + near - z * (far - near));
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

// Direct lighting calculation
vec3 CalculateDirectLighting(GBufferData gData, float shadow, vec3 lightDir, vec3 viewDir)
{
    vec3 halfDir = normalize(lightDir + normalize(u_CameraPos - gData.worldPos));

    vec3 F = fresnelSchlickRoughness(max(dot(viewDir, halfDir), 0.0), gData.F0, gData.roughness);
    float NDF = ggxNDF(max(dot(gData.normal, halfDir), 0.0), gData.roughness);
    float G = geometrySmith(max(dot(gData.normal, normalize(u_CameraPos - gData.worldPos)), 0.0), max(dot(gData.normal, lightDir), 0.0), gData.roughness);

    vec3 specular = F * NDF * G / (4.0 * max(dot(gData.normal, normalize(u_CameraPos - gData.worldPos)), 0.01) * max(dot(gData.normal, lightDir), 0.01));
    vec3 diffuse = (1.0 - F) * gData.albedo / 3.14159265359;

    return (diffuse + specular) * u_LightColor * max(dot(gData.normal, lightDir), 0.0) * shadow;
}

// Diffuse IBL calculation
vec3 CalculateDiffuseIBL(vec3 normal, vec3 albedo)
{
    vec3 irradiance = texture(gIrradianceMap, normal).rgb;
    return irradiance * albedo * 1.0;
}

// Specular IBL calculation
vec3 CalculateSpecularIBL(vec3 normal, vec3 viewDir, float roughness, vec3 F0)
{
    vec3 reflection = reflect(-viewDir, normal);
    vec3 prefilteredColor = textureLod(gPrefilteredMap, reflection, roughness * 4.0).rgb;
    float NdotV = max(dot(normal, viewDir), 0.0);
    vec2 brdf = texture(gBRDFLUT, vec2(NdotV, roughness)).rg;

    return prefilteredColor * (F0 * brdf.x + brdf.y) * 1.0;
}

// Reconstruct world position from depth
vec3 ReconstructWorldPosition(vec2 texCoords, float depth) 
{
    float ndcDepth = depth * 2.0 - 1.0; // Convert depth to NDC
    vec4 clipSpacePos = vec4(texCoords * 2.0 - 1.0, ndcDepth, 1.0);
    vec4 viewSpacePos = u_ProjectionInverse * clipSpacePos;
    viewSpacePos /= viewSpacePos.w; 
    vec4 worldSpacePos = u_ViewInverse * viewSpacePos;
    return worldSpacePos.xyz;
}

GBufferData ExtractGBufferData(vec2 texCoords)
{
    GBufferData gData;

    vec4 normalSample = texture(gNormals, texCoords);
    gData.albedo = texture(gAlbedoAO, texCoords).rgb;
    gData.ao = texture(gAlbedoAO, texCoords).a;
    gData.normal = normalize(normalSample.xyz * 2.0 - 1.0);
    gData.worldPos = ReconstructWorldPosition(texCoords, texture(gDepth, texCoords).r);
    vec2 metallicRoughness = texture(gMetallicRoughness, texCoords).rg;
    gData.metallic = metallicRoughness.r;
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

void main() 
{
    // Sample G-buffer
    float depth = texture(gDepth, v_TexCoords).r;

    //float linearDepth = LinearizeDepth(depth, u_Near, u_Far);
    vec3 worldPos = ReconstructWorldPosition(v_TexCoords, depth);
    vec3 viewDir = normalize(u_CameraPos - worldPos);

    // no depth? Render skybox
    if (depth == 1.0)
    {
        FragColor = texture(gSkyTexture, -viewDir);
        return;
    }

    GBufferData gData = ExtractGBufferData(v_TexCoords);

    vec3 lightDir = normalize(-u_LightDirection);

    vec4 fragPosLightSpace = u_LightSpaceMatrix * vec4(worldPos, 1.0);
    float shadow = 1.0;
    if (gData.receiveShadows > 0.0)
    {
        shadow = ShadowCalculation(fragPosLightSpace, gData.normal, lightDir);
    }

    vec3 lighting = CalculateDirectLighting(gData, shadow, lightDir, viewDir);
    vec3 iblDiffuse = CalculateDiffuseIBL(gData.normal, gData.albedo);
    vec3 iblSpecular = CalculateSpecularIBL(gData.normal, viewDir, gData.roughness, gData.F0);

    vec3 totalLighting = lighting + iblDiffuse + iblSpecular;
    totalLighting += gData.emissive;

    //totalLighting = ApplyToneMapping(totalLighting);
    //totalLighting = ApplyGammaCorrection(totalLighting);

    FragColor = vec4(totalLighting, 1.0);
}
