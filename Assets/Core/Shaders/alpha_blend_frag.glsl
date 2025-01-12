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

layout(std430, binding = 0) readonly buffer MaterialBuffer 
{
    MaterialGPU materials[];
};

uniform float u_SpecularIndirectFactor;
uniform float u_DiffuseIndirectFactor;

layout(binding = 0) uniform samplerCube u_IrradianceMap;
layout(binding = 1) uniform samplerCube u_PrefilteredMap;
layout(binding = 2) uniform sampler2D u_BRDFLUT;

out vec4 fragColor;

// Inputs from vertex shader
in vec3 v_Normal;        
in vec3 v_CameraPos;
in vec4 v_WorldPos;
in vec2 v_TexCoord;     
in vec2 v_ScreenCoord;
flat in uint v_MaterialIndex;

vec4 getBaseColor(MaterialGPU material) 
{
    bool hasTex = (material.baseColorHandle.x != 0 || material.baseColorHandle.y != 0);
    if (hasTex) 
    {
        return texture(sampler2D(material.baseColorHandle), v_TexCoord) * material.baseColorFactor;
    }
    return material.baseColorFactor;
}

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness) 
{
    // return F0 + (vec3(1.0) - F0) * pow(1.0 - cosTheta, 5.0);
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}

// Diffuse IBL calculation
vec3 CalculateDiffuseIBL(vec3 normal, vec3 albedo)
{
    vec3 irradiance = texture(u_IrradianceMap, normal).rgb;
    return irradiance * albedo * u_DiffuseIndirectFactor;
}

vec3 CalculateSpecularIBL(vec3 normal, vec3 viewDir, float roughness, vec3 F0)
{
    vec3 reflection = reflect(-viewDir, normal);
    vec3 prefilteredColor = textureLod(u_PrefilteredMap, reflection, roughness * 4.0).rgb;
    float NdotV = max(dot(normal, viewDir), 0.0);
    vec2 brdf = texture(u_BRDFLUT, vec2(NdotV, roughness)).rg;

    return prefilteredColor * (F0 * brdf.x + brdf.y) * u_SpecularIndirectFactor;
}

void main() 
{
    MaterialGPU material = materials[v_MaterialIndex];
    vec4 color = getBaseColor(material);

    // View direction (from camera to fragment)
    vec3 viewDir = normalize(v_CameraPos - v_WorldPos.xyz);  // Adjust according to your camera setup

    // Normal calculation
    vec3 normal = normalize(v_Normal);

    // need to finish implementing reflection/refraction math here

    fragColor = color;
}