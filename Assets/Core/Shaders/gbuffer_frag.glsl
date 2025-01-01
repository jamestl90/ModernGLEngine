#version 460 core

#extension GL_ARB_bindless_texture : require
//#extension GL_EXT_nonuniform_qualifier : enable
//#extension GL_NV_gpu_shader5 : enable

struct MaterialGPU 
{
    vec4 baseColorFactor;
    vec4 emissiveFactor;
    uvec2 baseColorHandle;
    uvec2 metallicRoughnessHandle;
    uvec2 normalHandle;
    uvec2 occlusionHandle;
    uvec2 emissiveHandle;
    float metallicFactor;
    float roughnessFactor;
    float alphaCutoff;
    uint alphaMode;
    uint receiveShadows;
};

// Outputs to G-buffer
layout(location = 0) out vec4 gAlbedoAO;          // Albedo (RGB) + AO (A)
layout(location = 1) out vec4 gNormalShadow;      // Normal (RGB) + ShadowInfo
layout(location = 2) out vec2 gMetallicRoughness; // Metallic (R) + Roughness (G)
layout(location = 3) out vec4 gEmissive;          // Emissive (RGB) + Reserved (A)

layout(std430, binding = 0) readonly buffer MaterialBuffer 
{
    MaterialGPU materials[];
};

// Inputs from vertex shader
in vec3 v_Normal;        
in vec2 v_TexCoord;     
in vec3 v_Tangent;       
in vec3 v_Bitangent;     
flat in uint v_MaterialIndex;

// Get material properties
vec4 getBaseColor(MaterialGPU material) 
{
    bool hasTex = (material.baseColorHandle.x != 0 || material.baseColorHandle.y != 0);
    if (hasTex) 
    {
        return texture(sampler2D(material.baseColorHandle), v_TexCoord) * material.baseColorFactor;
    }
    return material.baseColorFactor;
}

vec2 getMetallicRoughness(MaterialGPU material) 
{
    bool hasTex = (material.metallicRoughnessHandle.x != 0 || material.metallicRoughnessHandle.y != 0);
    if (hasTex) 
    {
        vec4 texValue = texture(sampler2D(material.metallicRoughnessHandle), v_TexCoord);
        float metallic = texValue.b * material.metallicFactor;
        float roughness = texValue.g * material.roughnessFactor;
        return vec2(metallic, roughness);
    }
    return vec2(material.metallicFactor, material.roughnessFactor);
}

vec3 getNormal(MaterialGPU material) 
{
    bool hasTex = (material.normalHandle.x != 0 || material.normalHandle.y != 0);
    if (hasTex) 
    {
        vec3 normalTex = texture(sampler2D(material.normalHandle), v_TexCoord).rgb;
        normalTex = normalTex * 2.0 - 1.0; // Map [0, 1] to [-1, 1]
        mat3 TBN = mat3(normalize(v_Tangent), normalize(v_Bitangent), normalize(v_Normal));
        return normalize(TBN * normalTex);
    }
    return normalize(v_Normal);
}

float GetAmbientOcclusion(MaterialGPU material)
{
    bool hasTex = (material.occlusionHandle.x != 0 || material.occlusionHandle.y != 0);
    if (hasTex) 
    {
        return texture(sampler2D(material.occlusionHandle), v_TexCoord).r;
    }
    return 1.0;
}

vec3 getEmissive(MaterialGPU material) 
{
    bool hasTex = (material.emissiveHandle.x != 0 || material.emissiveHandle.y != 0);
    if (hasTex) 
    {
        vec3 emissiveTexColor = texture(sampler2D(material.emissiveHandle), v_TexCoord).rgb;
        return material.emissiveFactor.rgb * emissiveTexColor;
    }
    return material.emissiveFactor.rgb;
}

void main() 
{
    MaterialGPU material = materials[v_MaterialIndex];

    vec4 baseColor = getBaseColor(material);
    vec2 metallicRoughness = getMetallicRoughness(material);
    vec3 normal = getNormal(material);
    vec3 emissive = getEmissive(material);
    float ao = GetAmbientOcclusion(material);

    if (material.alphaMode == 1 && baseColor.a < material.alphaCutoff) 
    {
        discard;
    }

    gAlbedoAO = vec4(baseColor.rgb, ao);          // Albedo + Ambient Occlusion
    gNormalShadow = vec4(normal, material.receiveShadows); // Encoded Normal + Shadow Info
    gMetallicRoughness = metallicRoughness;       // Metallic + Roughness
    gEmissive = vec4(emissive, 0.0);                   // Emissive + Reserved
}
