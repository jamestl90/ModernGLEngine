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
uniform vec4 baseColorFactor = vec4(1.0);          // Base color factor (RGBA)
uniform sampler2D baseColorTexture;               // Base color texture
uniform bool useBaseColorTexture = false;         // Flag to use texture

uniform float metallicFactor = 0.0;               // Metallic scalar
uniform float roughnessFactor = 0.0;              // Roughness scalar
uniform sampler2D metallicRoughnessTexture;       // Metallic-Roughness combined texture
uniform bool useMetallicRoughnessTexture = false; // Flag to use texture

uniform sampler2D normalTexture;                  // Normal map
uniform bool useNormalTexture = false;            // Flag to use texture

uniform sampler2D occlusionTexture;               // Ambient occlusion map
uniform bool useOcclusionTexture = false;         // Flag to use texture

uniform sampler2D emissiveTexture;                // Emissive map
uniform bool useEmissiveTexture = false;          // Flag to use texture
uniform vec3 emissiveFactor = vec3(0.0);          // Emissive factor

// Constants 
const float DEFAULT_AO = 1.0;                    // Default ambient occlusion

// Get material properties
vec4 getBaseColor() 
{
    if (useBaseColorTexture) 
    {
        vec4 textureColor = texture(baseColorTexture, v_TexCoords);
        return baseColorFactor * textureColor;
    }
    return baseColorFactor;
}

vec2 getMetallicRoughness() 
{
    if (useMetallicRoughnessTexture) 
    {
        vec4 texValue = texture(metallicRoughnessTexture, v_TexCoords);
        float metallic = texValue.b * metallicFactor;   // Metallic is in the B channel
        float roughness = texValue.g * roughnessFactor; // Roughness is in the G channel
        return vec2(metallic, roughness);
    }
    return vec2(metallicFactor, roughnessFactor);
}

vec3 getNormal() 
{
    if (useNormalTexture) 
    {
        vec3 normalTex = texture(normalTexture, v_TexCoords).rgb;
        normalTex = normalTex * 2.0 - 1.0; // Map [0, 1] to [-1, 1]

        // Construct TBN matrix from interpolated data
        mat3 TBN = mat3(normalize(v_Tangent), normalize(v_Bitangent), normalize(v_Normal));

        // Transform the normal map's tangent-space normal to world space
        vec3 worldNormal = normalize(TBN * normalTex);
        return worldNormal;
    }
    return normalize(v_Normal); // Use interpolated normal if no normal map
}

float getAmbientOcclusion() 
{
    if (useOcclusionTexture) 
    {
        return texture(occlusionTexture, v_TexCoords).r;
    }
    return DEFAULT_AO; // Default AO value
}

vec3 getEmissive() 
{
    if (useEmissiveTexture) 
    {
        vec3 emissiveTexColor = texture(emissiveTexture, v_TexCoords).rgb;
        return emissiveFactor * emissiveTexColor;
    }
    return emissiveFactor;
}

void main() 
{
    // Retrieve material properties
    vec4 baseColor = getBaseColor();
    vec2 metallicRoughness = getMetallicRoughness();
    vec3 normal = getNormal();
    float ao = getAmbientOcclusion();
    vec3 emissive = getEmissive();

    // Write to G-buffer
    gAlbedoAO = vec4(baseColor.rgb, ao);          // Albedo + Ambient Occlusion
    gNormalSmooth = vec4(normal, 1.0 - metallicRoughness.y); // Normal + Smoothness
    gMetallicRoughness = metallicRoughness;       // Metallic + Roughness
    gEmissive = vec4(emissive, 0.0);              // Emissive + Reserved
}
