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
    float transmissionFactor;             
    float refractionIndex;                
    float thickness;                      
    float padding1;                       
    vec3 attenuationColor;           
    float attenuationDistance;    
};

// Outputs to G-buffer
layout(location = 0) out vec4 gAlbedoAO;          // Albedo (RGB) + AO (A)
layout(location = 1) out vec4 gNormalShadow;      // Normal (RGB) + ShadowInfo
layout(location = 2) out vec4 gMetallicRoughness; // Metallic (R) + Roughness (G) (check gltf format)
layout(location = 3) out vec4 gEmissive;          // Emissive (RGB) + Reserved (A)
layout(location = 4) out vec3 gPositions;          // World Positions (RGB)
layout(location = 5) out float gLinearDepth;              // linear depth;

layout(std430, binding = 0) readonly buffer MaterialBuffer 
{
    MaterialGPU materials[];
};

layout(std140, binding = 2) uniform ShaderGlobalData 
{
    mat4 viewMatrix;
    mat4 projMatrix;
    vec4 camPos;
    vec4 camDir;
    vec2 timeInfo;
    vec2 windowSize;
    int frameCount;
};

// Inputs from vertex shader
in vec3 v_WorldPos;
in vec3 v_Normal;        
in vec2 v_TexCoord;     
in vec3 v_Tangent;       
in vec3 v_Bitangent;    
in float v_NegViewPosZ; 
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

vec4 getMetallicRoughness(MaterialGPU material) 
{
    bool hasTex = (material.metallicRoughnessHandle.x != 0 || material.metallicRoughnessHandle.y != 0);
    if (hasTex) 
    {
        vec4 texValue = texture(sampler2D(material.metallicRoughnessHandle), v_TexCoord);
        float metallic = texValue.g * material.metallicFactor;
        float roughness = texValue.b * material.roughnessFactor;
        float ao = texValue.r;
        return vec4(ao, metallic, roughness, 0.0);
    }
    return vec4(0.0f, material.roughnessFactor, material.metallicFactor, 1.0f);
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
    return material.emissiveFactor.rgb * material.emissiveFactor.a;
}

void main() 
{
    MaterialGPU material = materials[v_MaterialIndex];

    vec4 baseColor = getBaseColor(material);
    vec4 metallicRoughness = getMetallicRoughness(material);
    vec3 normal = getNormal(material);
    vec3 emissive = getEmissive(material);
    float ao = GetAmbientOcclusion(material);

    if (material.alphaMode == 1 && baseColor.a < material.alphaCutoff) 
    {
        discard;
    }

    vec3 viewNormal = normalize((viewMatrix * vec4(normal, 0.0)).xyz);

    gAlbedoAO = vec4(baseColor.rgb, ao);          // Albedo + Ambient Occlusion
    gNormalShadow = vec4(viewNormal, material.receiveShadows); // Encoded Normal + Shadow Info
    gMetallicRoughness = metallicRoughness;       // Metallic + Roughness
    gEmissive = vec4(emissive, 0.0);                   // Emissive + Reserved
    gPositions = v_WorldPos;
    gLinearDepth = v_NegViewPosZ;
}
