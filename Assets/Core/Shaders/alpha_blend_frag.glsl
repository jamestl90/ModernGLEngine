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

out vec4 fragColor;

// Inputs from vertex shader
in vec3 v_Normal;        
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

void main() 
{
    MaterialGPU material = materials[v_MaterialIndex];
    vec4 color = getBaseColor(material);
    fragColor = color;
}