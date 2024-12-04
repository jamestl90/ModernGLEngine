#version 460 core

in vec2 v_TexCoords;

uniform sampler2D gAlbedoAO;
uniform sampler2D gNormals;
uniform sampler2D gMetallicRoughness;
uniform sampler2D gEmissive;
uniform sampler2D gDepth;

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

vec3 calculatePBR(vec3 albedo, vec3 normal, vec3 lightDir, vec3 viewDir, vec3 lightColor, float metallic, float roughness) 
{
    vec3 F0 = mix(vec3(0.04), albedo, metallic); // Base reflectivity at normal incidence
    vec3 halfDir = normalize(lightDir + viewDir);

    // Geometry term (simplified)
    float NDF = max(dot(normal, lightDir), 0.0);
    float G = max(dot(normal, viewDir), 0.0);
    vec3 F = fresnelSchlick(max(dot(viewDir, halfDir), 0.0), F0);

    // Diffuse and specular
    vec3 diffuse = (1.0 - F) * albedo / 3.14159265359;
    vec3 specular = F * NDF * G / (4.0 * max(dot(normal, lightDir), 0.1) * max(dot(normal, viewDir), 0.1));

    return (diffuse + specular) * lightColor;
}

void main() 
{
    float depth = texture(gDepth, v_TexCoords).r;
    float linearDepth = LinearizeDepth(depth);
    vec3 worldPos = ReconstructWorldPosition(v_TexCoords, linearDepth);

    vec3 albedo = texture(gAlbedoAO, v_TexCoords).rgb;
    vec3 normal = normalize(texture(gNormals, v_TexCoords).rgb * 2.0 - 1.0); // Decode normal
    vec2 metallicRoughness = texture(gMetallicRoughness, v_TexCoords).rg;
    float metallic = metallicRoughness.r;
    float roughness = metallicRoughness.g;

    vec3 lightDir = normalize(-lightDirection);
    vec3 viewDir = normalize(cameraPos - worldPos); 

    vec3 lighting = calculatePBR(albedo, normal, lightDir, viewDir, lightColor, metallic, roughness);

    lighting += ambientColor * albedo; 
    lighting += texture(gEmissive, v_TexCoords).rgb;
    FragColor = vec4(lighting, 1.0);
}
