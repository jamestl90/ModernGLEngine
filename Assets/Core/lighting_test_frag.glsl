#version 460 core

#define DEBUG_OUTPUT = 0

in vec2 v_TexCoords;

layout(binding = 0) uniform sampler2D gAlbedoAO;
layout(binding = 1) uniform sampler2D gNormals;
layout(binding = 2) uniform sampler2D gMetallicRoughness;
layout(binding = 3) uniform sampler2D gEmissive;
layout(binding = 4) uniform sampler2D gDepth;

uniform float u_Near;
uniform float u_Far;

uniform vec3 lightDirection; // Directional light direction (normalized)
uniform vec3 lightColor;     // Directional light color
uniform vec3 cameraPos;      // Camera position for view direction

uniform mat4 u_ViewInverse;
uniform mat4 u_ProjectionInverse;

out vec4 FragColor;

const vec3 ambientColor = vec3(0.03); // Low ambient lighting

float LinearizeDepth(float depth) 
{
    float z = depth * 2.0 - 1.0; // Convert to NDC space
    return (2.0 * u_Near * u_Far) / (u_Far + u_Near - z * (u_Far - u_Near));
}

vec3 ReconstructWorldPosition(vec2 texCoords, float depth) 
{
    vec4 ndcPos = vec4((texCoords * 2.0 - 1.0), depth * 2.0 - 1.0, 1.0);
    vec4 viewPos = u_ProjectionInverse * ndcPos;
    viewPos /= viewPos.w;
    vec4 worldPos = u_ViewInverse * viewPos;
    return worldPos.xyz;
}

// Fresnel-Schlick approximation for specular reflection
vec3 fresnelSchlick(float cosTheta, vec3 F0) 
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

float ggxNDF(float NdotH, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH2 = NdotH * NdotH;

    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    return a2 / (3.14159265359 * denom * denom);
}

float schlickGGX_G(float NdotV, float roughness) {
    float k = (roughness + 1.0) * (roughness + 1.0) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

vec3 calculatePBR(vec3 albedo, vec3 normal, vec3 lightDir, vec3 viewDir, vec3 lightColor, float metallic, float roughness) {
    vec3 F0 = mix(vec3(0.04), albedo, metallic);
    vec3 halfDir = normalize(lightDir + viewDir);

    // Fresnel term
    vec3 F = fresnelSchlick(max(dot(viewDir, halfDir), 0.0), F0);

    // NDF
    float NDF = ggxNDF(max(dot(normal, halfDir), 0.0), roughness);

    // Geometry
    float G = schlickGGX_G(max(dot(normal, lightDir), 0.0), roughness) *
              schlickGGX_G(max(dot(normal, viewDir), 0.0), roughness);

    // Specular
    vec3 specular = F * NDF * G / (4.0 * max(dot(normal, lightDir), 0.01) * max(dot(normal, viewDir), 0.01));

    // Diffuse
    vec3 diffuse = (1.0 - F) * albedo / 3.14159265359;

    return (diffuse + specular) * lightColor;
}
void main() 
{
    float depth = texture(gDepth, v_TexCoords).r;
    float linearDepth = LinearizeDepth(depth);
    vec3 worldPos = ReconstructWorldPosition(v_TexCoords, linearDepth);

    vec3 albedo = texture(gAlbedoAO, v_TexCoords).rgb;
    vec3 normal = normalize(texture(gNormals, v_TexCoords).rgb); // Decode normal
    vec2 metallicRoughness = texture(gMetallicRoughness, v_TexCoords).rg;
    float metallic = metallicRoughness.r;
    float roughness = metallicRoughness.g;

    vec3 lightDir = normalize(-lightDirection);
    vec3 viewDir = normalize(cameraPos); 

    vec3 lighting = calculatePBR(albedo, normal, lightDir, viewDir, lightColor, metallic, roughness);

    lighting += ambientColor * albedo; 
    FragColor = vec4(lighting, 1.0);
}
