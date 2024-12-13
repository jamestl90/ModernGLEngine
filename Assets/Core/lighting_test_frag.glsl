#version 460 core

in vec2 v_TexCoords;

layout(binding = 0) uniform sampler2D gAlbedoAO;
layout(binding = 1) uniform sampler2D gNormals;
layout(binding = 2) uniform sampler2D gMetallicRoughness;
layout(binding = 3) uniform sampler2D gEmissive;
layout(binding = 4) uniform sampler2D gDepth;
layout(binding = 5) uniform sampler2D gDLShadowMap;

uniform mat4 u_LightSpaceMatrix; // Combined light projection and view matrix
uniform mat4 u_ViewInverse;
uniform mat4 u_ProjectionInverse;

uniform float u_Bias;

uniform float u_Near;
uniform float u_Far;

uniform vec3 u_LightDirection; // Directional light direction (normalized)
uniform vec3 u_LightColor;     // Directional light color
uniform vec3 u_CameraPos;      // Camera position for view direction

out vec4 FragColor;

const vec3 ambientColor = vec3(0.05); // Low ambient lighting

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

    // Check if the fragment is outside the shadow map bounds
    if (projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0) {
        return 1.0; // Outside the shadow map, fully lit
    }

    // Sample depth from shadow map
    float closestDepth = texture(gDLShadowMap, projCoords.xy).r;

    // Current fragment depth in light space
    float currentDepth = projCoords.z;

    // Bias to prevent shadow acne
    float bias = clamp(max(u_Bias * (1.0 - dot(normal, lightDir)), 0.0001), 0.0, 0.01);

    // Basic shadow test
    float shadow = (currentDepth - bias) > closestDepth ? 0.0 : 1.0;

    // Optional: Percentage Closer Filtering (PCF)
    float pcfShadow = 0.0;
    float texelSize = 1.0 / 4096.0;
    for (int x = -2; x <= 2; ++x) {
        for (int y = -2; y <= 2; ++y) {
            vec2 offset = vec2(x, y) * texelSize;
            float pcfDepth = texture(gDLShadowMap, projCoords.xy + offset).r;
            pcfShadow += (currentDepth - bias) > pcfDepth ? 0.0 : 1.0;
        }
    }
    shadow = pcfShadow / 25.0; // Average the 3x3 PCF kernel

    return shadow;
}

// Fresnel-Schlick approximation for specular reflection
vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness) {
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}

// GGX Normal Distribution Function
float ggxNDF(float NdotH, float roughness) {
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

// PBR lighting model
vec3 calculatePBR(vec3 albedo, vec3 normal, vec3 lightDir, vec3 viewDir, vec3 lightColor, float metallic, float roughness, float ao)
{
    vec3 halfDir = normalize(lightDir + viewDir);
    vec3 F0 = mix(vec3(0.04), albedo, metallic);
    vec3 F = fresnelSchlickRoughness(max(dot(viewDir, halfDir), 0.0), F0, roughness);

    float NDF = ggxNDF(max(dot(normal, halfDir), 0.0), roughness);
    float G = geometrySmith(max(dot(normal, viewDir), 0.0), max(dot(normal, lightDir), 0.0), roughness);
    vec3 specular = F * NDF * G / (4.0 * max(dot(normal, viewDir), 0.01) * max(dot(normal, lightDir), 0.01));
    vec3 diffuse = (1.0 - F) * albedo / 3.14159265359;

    vec3 lighting = (diffuse + specular) * lightColor * max(dot(normal, lightDir), 0.0);
    lighting *= ao; 

    return lighting;
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

void main() 
{
    // Sample G-buffer
    float depth = texture(gDepth, v_TexCoords).r;
    //float linearDepth = LinearizeDepth(depth, u_Near, u_Far);
    vec3 worldPos = ReconstructWorldPosition(v_TexCoords, depth);

    vec4 albedoAO = texture(gAlbedoAO, v_TexCoords);
    vec3 albedo = albedoAO.rgb;
    float ao = albedoAO.a;

    vec3 normal = normalize(texture(gNormals, v_TexCoords).xyz * 2.0 - 1.0); // Decode normal
    vec2 metallicRoughness = texture(gMetallicRoughness, v_TexCoords).rg;
    float metallic = metallicRoughness.r;
    float roughness = max(metallicRoughness.g, 0.05);

    vec3 lightDir = normalize(-u_LightDirection);
    vec3 viewDir = normalize(u_CameraPos - worldPos);

    vec3 lighting = calculatePBR(albedo, normal, lightDir, viewDir, u_LightColor, metallic, roughness, ao);

    vec4 fragPosLightSpace = u_LightSpaceMatrix * vec4(worldPos, 1.0);
    float shadow = ShadowCalculation(fragPosLightSpace, normal, lightDir);
    lighting *= shadow;

    lighting += ambientColor * albedo;
    lighting += texture(gEmissive, v_TexCoords).rgb * 1.15f;
    lighting *= 3.0;

    FragColor = vec4(lighting, 1.0);
}
