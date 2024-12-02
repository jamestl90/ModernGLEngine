#version 460 core

// Inputs from vertex shader
in vec3 v_FragPos;       // Fragment position in world space
in vec3 v_Normal;        // Normal in world space
in vec2 v_TexCoords;     // Texture coordinates
in vec3 v_Tangent;       // Tangent in world space
in vec3 v_Bitangent;     // Bitangent in world space

// Outputs to G-buffer
layout(location = 0) out vec4 gAlbedoAO;          // Albedo (RGB) + AO (A)
layout(location = 1) out vec4 gNormalSmooth;      // Normal (RGB) + Smoothness (A)
layout(location = 2) out vec2 gMetallicRoughness; // Metallic (R) + Roughness (G)
layout(location = 3) out vec4 gEmissive;          // Emissive (RGB) + Reserved (A)

// Material properties
uniform vec4 baseColorFactor;                  // Base color factor (RGBA)
uniform sampler2D baseColorTexture;            // Base color texture

uniform float metallicFactor;                  // Metallic scalar
uniform float roughnessFactor;                 // Roughness scalar
uniform sampler2D metallicRoughnessTexture;    // Metallic-Roughness combined texture

uniform sampler2D normalTexture;               // Normal map
uniform sampler2D occlusionTexture;            // Ambient occlusion map
uniform sampler2D emissiveTexture;             // Emissive map
uniform vec3 emissiveFactor;                   // Emissive factor

// Constants
const vec3 DEFAULT_NORMAL = vec3(0.0, 0.0, 1.0); // Default normal for flat surfaces

// Get material properties
vec4 getBaseColor() {
    vec4 textureColor = texture(baseColorTexture, v_TexCoords);
    return baseColorFactor * textureColor;
}

vec2 getMetallicRoughness() {
    vec4 texValue = texture(metallicRoughnessTexture, v_TexCoords);
    float metallic = texValue.b * metallicFactor;   // Metallic is in the B channel
    float roughness = texValue.g * roughnessFactor; // Roughness is in the G channel
    return vec2(metallic, roughness);
}

vec3 getNormal() {
    vec3 normalTex = texture(normalTexture, v_TexCoords).rgb;
    normalTex = normalTex * 2.0 - 1.0; // Map to range [-1, 1]
    mat3 TBN = mat3(normalize(v_Tangent), normalize(v_Bitangent), normalize(v_Normal));
    return normalize(TBN * normalTex); // Transform to world space
}

float getAmbientOcclusion() {
    return texture(occlusionTexture, v_TexCoords).r;
}

vec3 getEmissive() {
    vec3 emissiveTexColor = texture(emissiveTexture, v_TexCoords).rgb;
    return emissiveFactor * emissiveTexColor;
}

void main() {
    // Retrieve material properties
    vec4 baseColor = getBaseColor();
    vec2 metallicRoughness = getMetallicRoughness();
    vec3 normal = getNormal();
    float ao = getAmbientOcclusion();
    vec3 emissive = getEmissive();

    // Write to G-buffer
    gAlbedoAO = vec4(baseColor.rgb, ao);          // Albedo + Ambient Occlusion
    gNormalSmooth = vec4(normalize(normal), 1.0 - metallicRoughness.y); // Normal + Smoothness
    gMetallicRoughness = metallicRoughness;       // Metallic + Roughness
    gEmissive = vec4(emissive, 0.0);              // Emissive + Reserved
}
