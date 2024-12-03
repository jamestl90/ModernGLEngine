#version 460 core

// Uniforms for PBR Metallic-Roughness properties
uniform vec4 baseColorFactor;              // Base color (RGBA)
uniform sampler2D baseColorTexture;        // Texture for base color

uniform float metallicFactor;              // Metalness (0 = dielectric, 1 = metallic)
uniform float roughnessFactor;             // Surface roughness
uniform sampler2D metallicRoughnessTexture; // Combined metallic-roughness texture

uniform sampler2D normalTexture;           // Normal map
uniform sampler2D occlusionTexture;        // Ambient occlusion map
uniform sampler2D emissiveTexture;         // Emissive map
uniform vec3 emissiveFactor;               // Emissive color

// Alpha properties
uniform float alphaCutoff;                 // Alpha cutoff for "MASK" mode
uniform bool doubleSided;                  // Double-sided rendering flag

// Flags for alternative workflows
uniform bool usesSpecularGlossinessWorkflow;
uniform vec4 diffuseFactor;                // Diffuse color (for specular-glossiness)
uniform sampler2D diffuseTexture;          // Diffuse texture
uniform vec3 specularFactor;               // Specular color (for specular-glossiness)
uniform sampler2D specularGlossinessTexture; // Specular-glossiness texture

// Inputs from vertex shader
in vec3 Normal;       // Surface normal (normalized)
in vec3 FragPos;      // Fragment position in world space
in vec2 TexCoords;    // Texture coordinates

// Outputs
out vec4 FragColor;

// Constants
const float PI = 3.14159265359;

// Fresnel-Schlick approximation
vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

// Sample the base color
vec4 getBaseColor() {
    vec4 textureColor = texture(baseColorTexture, TexCoords);
    return baseColorFactor * textureColor;
}

// Sample the metallic and roughness values
vec2 getMetallicRoughness() {
    vec4 texValue = texture(metallicRoughnessTexture, TexCoords);
    float metallic = texValue.b * metallicFactor;  // Metallic is in the B channel
    float roughness = texValue.g * roughnessFactor; // Roughness is in the G channel
    return vec2(metallic, roughness);
}

// Sample the emissive color
vec3 getEmissiveColor() {
    vec3 emissiveTexColor = texture(emissiveTexture, TexCoords).rgb;
    return emissiveFactor * emissiveTexColor;
}

// Compute the normal from the normal map
vec3 getNormal() {
    vec3 normalTex = texture(normalTexture, TexCoords).rgb;
    normalTex = normalTex * 2.0 - 1.0; // Convert to range [-1, 1]
    return normalize(normalTex);
}

// Sample the ambient occlusion
float getOcclusion() {
    return texture(occlusionTexture, TexCoords).r;
}

// Compute the Fresnel reflectance (F0) based on material properties
vec3 getF0(vec3 albedo, float metallic) {
    return mix(vec3(0.04), albedo, metallic);
}

// Main lighting calculation
vec3 calculateLighting(vec3 N, vec3 V, vec3 L, vec3 albedo, float metallic, float roughness) {
    vec3 H = normalize(V + L); // Halfway vector

    // Geometry properties
    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.0);
    float HdotV = max(dot(H, V), 0.0);

    // Fresnel reflectance
    vec3 F0 = getF0(albedo, metallic);
    vec3 F = fresnelSchlick(HdotV, F0);

    // Sample precomputed BRDF LUT
    vec2 LUT = texture(brdfLUT, vec2(NdotV, roughness)).rg;

    // Compute diffuse and specular components
    vec3 kd = vec3(1.0) - F; // Energy conservation: kd + ks = 1
    kd *= 1.0 - metallic;    // Metallic surfaces have no diffuse reflection
    vec3 diffuse = kd * albedo / PI;
    vec3 specular = F * (LUT.x + LUT.y);

    // Combine lighting components
    return (diffuse + specular) * NdotL;
}

// Main function
void main() {
    // Retrieve material properties
    vec4 baseColor = getBaseColor();
    vec2 metallicRoughness = getMetallicRoughness();
    vec3 emissiveColor = getEmissiveColor();
    vec3 N = getNormal();
    float occlusion = getOcclusion();

    // Lighting setup
    vec3 V = normalize(viewDirection); // View direction
    vec3 L = normalize(lightDirection); // Light direction
    vec3 albedo = baseColor.rgb;
    float metallic = metallicRoughness.x;
    float roughness = metallicRoughness.y;

    // Calculate lighting
    vec3 lighting = calculateLighting(N, V, L, albedo, metallic, roughness);

    // Add emissive and occlusion effects
    lighting += emissiveColor;
    lighting *= occlusion;

    // Handle alpha masking
    if (baseColor.a < alphaCutoff) {
        discard;
    }

    // Set final fragment color
    FragColor = vec4(lighting, baseColor.a);
}