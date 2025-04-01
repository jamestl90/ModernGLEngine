#version 460 core

#extension GL_ARB_bindless_texture : require

layout(location = 0) in vec3 a_Position; 
layout(location = 1) in vec3 a_Normal;   
layout(location = 2) in vec2 a_TexCoord; 
layout(location = 3) in vec3 a_Tangent;  

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

struct PerDrawData 
{
    mat4 modelMatrix;
    uint materialIndex;
};

//layout(std430, binding = 0) readonly buffer MaterialBuffer 
//{
//    MaterialGPU materials[];
//};

layout(std430, binding = 1) readonly buffer PerDrawDataBuffer 
{
    PerDrawData perDrawData[];
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

out vec3 v_WorldPos;
out vec3 v_Normal;
out vec2 v_TexCoord;
out vec3 v_Tangent;
out vec3 v_Bitangent;
flat out uint v_MaterialIndex;

//float getHeight(MaterialGPU material) 
//{
//    bool hasTex = (material.metallicRoughnessHandle.x != 0 || material.metallicRoughnessHandle.y != 0);
//    if (hasTex) 
//    {
//        float texValue = texture(sampler2D(material.metallicRoughnessHandle), a_TexCoord).r;
//        return texValue;
//    }
//    return 0.0;
//}

void main() 
{
    PerDrawData data = perDrawData[gl_DrawID + gl_InstanceID];
    mat4 modelMatrix = data.modelMatrix;
    v_MaterialIndex = data.materialIndex;

    //MaterialGPU material = materials[v_MaterialIndex];
    //float height = getHeight(material);   

    mat3 normalMatrix = mat3(transpose(inverse(modelMatrix)));

    v_Normal = normalize(normalMatrix * a_Normal);
    v_Tangent = normalize(normalMatrix  * a_Tangent);
    v_Bitangent = normalize(cross(v_Normal, v_Tangent)); 

    v_TexCoord = a_TexCoord;

    //vec3 displacePos = a_Position;
    //if (height > 0)
    //{
    //    displacePos += (height * a_Normal);
    //}

    vec4 worldPosition = modelMatrix * vec4(a_Position, 1.0);
    v_WorldPos = worldPosition.xyz;
    gl_Position = projMatrix * viewMatrix * worldPosition;
}