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
layout(binding = 11) uniform sampler2D gLinearDepth;

uniform mat4 u_LightSpaceMatrix; // Combined light projection and view matrix
uniform mat4 u_ViewInverse;
uniform mat4 u_ProjectionInverse;

uniform int u_PCFKernelSize;
uniform float u_Bias;

uniform float u_SpecularIndirectFactor;
uniform float u_DiffuseIndirectFactor;
uniform float m_DirectFactor;

uniform float u_Near;
uniform float u_Far;

uniform vec3 u_LightDirection;
uniform vec3 u_LightColor;      

uniform vec3 u_DDGI_GridCenter;
uniform vec3 u_DDGI_ProbeSpacing;
uniform ivec3 u_DDGI_GridResolution;

// change these to uniforms later
const float u_DDGIVisibilityBias = 0.01;
const float u_DDGIVisibilitySharpness = 40.0;

struct DDGIProbe
{
    vec4 WorldPosition;     
    vec4 SHCoeffs[9];       
    float Depth;            
    float DepthMoment2;    
    vec2 padding; 
};

layout(std430, binding = 7) readonly buffer ProbeData 
{
    DDGIProbe probes[];
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
    vec3 worldPosFromDepth;
    float ao;
    float metallic;
    float roughness;
    vec3 F0;
    vec3 emissive;
    float receiveShadows;
    float depth;
    float linearDepth;
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
    gData.worldPos = texture(gPositions, texCoords).xyz; 
    gData.worldPosFromDepth = ReconstructWorldPosFromDepth(texCoords, gData.depth);
    vec4 metallicRoughness = texture(gMetallicRoughness, texCoords);
    gData.metallic = metallicRoughness.b;
    gData.roughness = max(metallicRoughness.g, 0.05);
    gData.F0 = mix(vec3(0.04), gData.albedo, gData.metallic);
    gData.emissive = texture(gEmissive, texCoords).rgb;
    gData.receiveShadows = normalSample.w;

    return gData;
}

// DDGI Functions
int Flatten3DIndex(ivec3 coord, ivec3 resolution) 
{
    return coord.x + coord.y * resolution.x + coord.z * resolution.x * resolution.y;
}

int GetProbeIndex(ivec3 coords, ivec3 gridResolution) 
{ 
    ivec3 clampedCoords = clamp(coords, ivec3(0), gridResolution - 1);
    return Flatten3DIndex(clampedCoords, gridResolution);
}

vec3 EvaluateSH9(vec4 shCoeffs[9], vec3 direction)
{
    vec3 n = normalize(direction);
    float x = n.x;
    float y = n.y;
    float z = n.z;

    // constants MUST match projection
    float Y[9];
    // L=0
    Y[0] = 0.2820947918; // Y00
    // L=1
    Y[1] = -0.4886025119 * y; // Y1-1
    Y[2] =  0.4886025119 * z; // Y10
    Y[3] = -0.4886025119 * x; // Y11
    // L=2
    Y[4] =  1.0925484306 * x * y; // Y2-2
    Y[5] = -1.0925484306 * y * z; // Y2-1
    Y[6] =  0.3153915652 * (3.0*z*z - 1.0); // Y20
    Y[7] = -1.0925484306 * x * z; // Y21
    Y[8] =  0.5462742153 * (x*x - y*y); // Y22

    // reconstruct irradiance E(n) = sum(Clm * Ylm(n))
    vec3 irradiance = vec3(0.0);
    for (int i = 0; i < 9; ++i)
    {
        irradiance += shCoeffs[i].rgb * Y[i];
    }

    return max(irradiance, vec3(0.0));
}

float CalculateProbeVisibility(vec3 worldPos, int probeIndex)
{
    DDGIProbe probe = probes[probeIndex];

    if (probe.Depth <= 0.01) 
    {
        return 0.0;
    }

    vec3 probePos = probe.WorldPosition.xyz;
    vec3 probeToPoint = worldPos - probePos;
    float dist = length(probeToPoint);

    dist = max(0.0, dist - u_DDGIVisibilityBias);

    float probeMeanDepth = probe.Depth;
    float probeMeanDepthSq = probe.DepthMoment2;

    float variance = probeMeanDepthSq - (probeMeanDepth * probeMeanDepth);
    variance = max(0.0, variance);

    float distDiff = max(0.0, dist - probeMeanDepth); 
    float chebyshev = variance / (variance + distDiff * distDiff);

    chebyshev = isnan(chebyshev) ? 1.0 : chebyshev; 

    float visibility = pow(smoothstep(0.0, 1.0, chebyshev), u_DDGIVisibilitySharpness);

    return visibility;
}


vec3 SampleDDGI(vec3 worldPos, vec3 normalWS)
{
    vec3 posRelativeToCenter = worldPos - u_DDGI_GridCenter;

    vec3 probeGridTotalSize = vec3(u_DDGI_GridResolution - 1) * u_DDGI_ProbeSpacing;
    probeGridTotalSize = max(probeGridTotalSize, vec3(1e-5));

    vec3 normalizedPos_NegHalfToPosHalf = posRelativeToCenter / probeGridTotalSize;

    vec3 normalizedPos_Indices = normalizedPos_NegHalfToPosHalf * vec3(u_DDGI_GridResolution - 1);
    vec3 probeSpacePos = normalizedPos_Indices + vec3(u_DDGI_GridResolution - 1) * 0.5;

    ivec3 baseCoords = ivec3(floor(probeSpacePos));
    vec3 lerpFactors = fract(probeSpacePos);

    vec3 totalIrradiance = vec3(0.0);
    float totalVisibilityWeight = 0.0;

    for (int z = 0; z < 2; ++z) 
    {
        for (int y = 0; y < 2; ++y) 
        {
            for (int x = 0; x < 2; ++x) 
            {
                ivec3 cornerOffset = ivec3(x, y, z);
                ivec3 probeCoords = baseCoords + cornerOffset;

                int probeIndex = GetProbeIndex(probeCoords, u_DDGI_GridResolution);
                float visibility = 1.0; // CalculateProbeVisibility(worldPos, probeIndex);

                if (visibility > 1e-5)
                {
                    vec3 probeIrradiance = EvaluateSH9(probes[probeIndex].SHCoeffs, normalWS);

                    vec3 weight3D = vec3(cornerOffset);
                    float weight = mix(1.0 - lerpFactors.x, lerpFactors.x, weight3D.x) *
                                   mix(1.0 - lerpFactors.y, lerpFactors.y, weight3D.y) *
                                   mix(1.0 - lerpFactors.z, lerpFactors.z, weight3D.z);

                    totalIrradiance += probeIrradiance * visibility * weight;
                    totalVisibilityWeight += visibility * weight;
                }
            }
        }
    }

    if (totalVisibilityWeight > 1e-5) 
    {
        return totalIrradiance / totalVisibilityWeight;
    } else 
    {
        return vec3(0.0);
    }
}

// main
void main() 
{
    GBufferData gData = ExtractGBufferData(v_TexCoords);

    if (gData.depth >= 0.9999) 
    {
        vec3 viewDirWS_Sky = normalize(gData.worldPosFromDepth - camPos.xyz);
        DirectLight = texture(gSkyTexture, viewDirWS_Sky).rgb; 
        IBL = vec3(0.0);
        IndirectLight   = vec3(0.0);
        return;
    }

    // Calculate necessary vectors in world space
    vec3 viewDirWS  = normalize(camPos.xyz - gData.worldPos);
    vec3 lightDirWS = normalize(-u_LightDirection);
    vec3 normalWS   = normalize((u_ViewInverse * vec4(gData.normal, 0.0)).xyz);

    // --- Shadows ---
    vec4 fragPosLightSpace = u_LightSpaceMatrix * vec4(gData.worldPos, 1.0);
    float shadow = 1.0;
    if (gData.receiveShadows > 0.0) 
    {
        shadow = ShadowCalculation(fragPosLightSpace, normalWS, lightDirWS);
    }

    // --- Direct Lighting ---
    vec3 halfDirWS      = normalize(lightDirWS + viewDirWS);
    float NdotL_WS      = max(dot(normalWS, lightDirWS), 0.0);
    float NdotV_WS      = max(dot(normalWS, viewDirWS), 0.0);
    float NdotH_WS      = max(dot(normalWS, halfDirWS), 0.0);
    float VdotH_WS      = max(dot(viewDirWS, halfDirWS), 0.0);
  
    vec3 F              = fresnelSchlickRoughness(VdotH_WS, gData.F0, gData.roughness);
    float NDF           = ggxNDF(NdotH_WS, gData.roughness);
    float G             = geometrySmith(NdotV_WS, NdotL_WS, gData.roughness);
    vec3 specularDirect = F * NDF * G / max(4.0 * NdotV_WS * NdotL_WS, 0.001);
    vec3 kD             = (vec3(1.0) - F) * (1.0 - gData.metallic); // Diffuse factor
    vec3 diffuseDirect  = kD * gData.albedo / 3.14159;
    vec3 directLighting = (diffuseDirect + specularDirect) * u_LightColor * NdotL_WS * shadow;

    // --- Indirect Lighting ---

    // Specular IBL (Still use global prefiltered map)
    vec3 reflectionWS     = reflect(-viewDirWS, normalWS);
    vec3 prefilteredColor = textureLod(gPrefilteredMap, reflectionWS, gData.roughness * 4.0).rgb;
    vec2 brdf             = texture(gBRDFLUT, vec2(NdotV_WS, gData.roughness)).rg;
    // Use same Fresnel as direct lighting for consistency
    vec3 specularIBL      = prefilteredColor * (F * brdf.x + brdf.y); // F = Fresnel calculated earlier
    // Diffuse GI (Use DDGI result exclusively)
    vec3 ddgiIrradiance   = SampleDDGI(gData.worldPos, normalWS);
    vec3 diffuseGI        = ddgiIrradiance * kD * gData.albedo / 3.14159;

    // Apply scaling factors (optional, can be done in combine pass too)
    diffuseGI            *= u_DiffuseIndirectFactor;
    specularIBL          *= u_SpecularIndirectFactor;

    // --- Final Output Assignment (Option 1 Structure) ---
    // AO affects all lighting components
    DirectLight     = (directLighting + gData.emissive) * gData.ao * m_DirectFactor;
    IBL             = specularIBL * gData.ao;
    IndirectLight   = diffuseGI * gData.ao * 10;
}
