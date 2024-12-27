#version 460 core
#extension GL_ARB_bindless_texture : require
#extension GL_NV_gpu_shader5 : enable

// Outputs to G-buffer
layout(location = 0) out vec4 gAlbedoAO;          // Albedo (RGB) + AO (A)
layout(location = 1) out vec4 gNormalShadow;      // Normal (RGB) + ShadowInfo
layout(location = 2) out vec2 gMetallicRoughness; // Metallic (R) + Roughness (G)
layout(location = 3) out vec4 gEmissive;          // Emissive (RGB) + Reserved (A)

struct MaterialGPU {
    vec4 baseColorFactor;
    vec4 emissiveFactor;
    float metallicFactor;
    float roughnessFactor;
    float alphaCutoff;
    int baseColorTextureIndex;
    int metallicRoughnessTextureIndex;
    int normalTextureIndex;
    int occlusionTextureIndex;
    int emissiveTextureIndex;
    int alphaMode;
    int doubleSided;
    int castShadows;
    int receiveShadows;
    int padding;
};

layout(std430, binding = 0) buffer MaterialBuffer {
    MaterialGPU materials[];
};

layout(std430, binding = 1) buffer TextureBuffer {
    uint64_t textureHandles[];            // Array of bindless texture handles
};

// Inputs from vertex shader
in vec3 v_Normal;        
in vec2 v_TexCoord;     
in vec3 v_Tangent;       
in vec3 v_Bitangent;     
flat in int v_MaterialID;

// Get material properties
vec4 getBaseColor(MaterialGPU material) 
{
    if (material.baseColorTextureIndex >= 0) {
        return texture(sampler2D(textureHandles[material.baseColorTextureIndex]), v_TexCoord) * material.baseColorFactor;
    }
    return material.baseColorFactor;
}

vec2 getMetallicRoughness(MaterialGPU material) 
{
    if (material.metallicRoughnessTextureIndex >= 0) {
        vec4 texValue = texture(sampler2D(textureHandles[material.metallicRoughnessTextureIndex]), v_TexCoord);
        float metallic = texValue.r * material.metallicFactor;
        float roughness = texValue.g * material.roughnessFactor;
        return vec2(metallic, roughness);
    }
    return vec2(material.metallicFactor, material.roughnessFactor);
}

vec3 getNormal(MaterialGPU material) 
{
    if (material.normalTextureIndex >= 0) {
        vec3 normalTex = texture(sampler2D(textureHandles[material.normalTextureIndex]), v_TexCoord).rgb;
        normalTex = normalTex * 2.0 - 1.0; // Map [0, 1] to [-1, 1]
        mat3 TBN = mat3(normalize(v_Tangent), normalize(v_Bitangent), normalize(v_Normal));
        return normalize(TBN * normalTex);
    }
    return normalize(v_Normal);
}

vec3 getEmissive(MaterialGPU material) 
{
    if (material.emissiveTextureIndex >= 0) {
        vec3 emissiveTexColor = texture(sampler2D(textureHandles[material.emissiveTextureIndex]), v_TexCoord).rgb;
        return material.emissiveFactor.rgb * emissiveTexColor;
    }
    return material.emissiveFactor.rgb;
}

float encodeShadowFlags(int receiveShadows, int castShadows) 
{
    return float(castShadows) + 0.1 * float(receiveShadows);
}

void main() 
{
    MaterialGPU material = materials[v_MaterialID];

     vec4 baseColor = getBaseColor(material);
    vec2 metallicRoughness = getMetallicRoughness(material);
    vec3 normal = getNormal(material);
    vec3 emissive = getEmissive(material);

    float ao = 1.0;
    if (material.occlusionTextureIndex >= 0) {
        ao = texture(sampler2D(textureHandles[material.occlusionTextureIndex]), v_TexCoord).r;
    }

    // Handle alpha masking
    if (material.alphaMode == 1 && baseColor.a < material.alphaCutoff) {
        discard;
    }

    float shadowInfo = encodeShadowFlags(material.receiveShadows, material.castShadows);

    // Write to G-buffer
    gAlbedoAO = vec4(baseColor.rgb, ao);          // Albedo + Ambient Occlusion
    gNormalShadow = vec4(normal * 0.5 + 0.5, shadowInfo); // Encoded Normal + Shadow Info
    gMetallicRoughness = metallicRoughness;       // Metallic + Roughness
    gEmissive = vec4(emissive, 0.0);                   // Emissive + Reserved
}
