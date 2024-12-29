#version 460 core

layout(location = 0) in vec3 a_Position; 
layout(location = 1) in vec3 a_Normal;   
layout(location = 2) in vec2 a_TexCoord; 
layout(location = 3) in vec3 a_Tangent;  

struct PerDrawData 
{
    mat4 modelMatrix;
    uint materialIndex;
};

layout(std430, binding = 1) readonly buffer PerDrawDataBuffer 
{
    PerDrawData perDrawData[];
};

uniform mat4 u_View;
uniform mat4 u_Projection;

out vec3 v_Normal;
out vec2 v_TexCoord;
out vec3 v_Tangent;
out vec3 v_Bitangent;
flat out uint v_MaterialIndex;

void main() 
{
    PerDrawData data = perDrawData[gl_DrawID];
    mat4 modelMatrix = data.modelMatrix;
    v_MaterialIndex = data.materialIndex;

    mat3 normalMatrix = mat3(transpose(inverse(modelMatrix)));

    v_Normal = normalize(normalMatrix * a_Normal);
    v_Tangent = normalize(mat3(modelMatrix) * a_Tangent);
    v_Bitangent = normalize(cross(v_Normal, v_Tangent)); 

    v_TexCoord = a_TexCoord;

    // clip-space position
    vec4 worldPosition = modelMatrix * vec4(a_Position, 1.0);
    gl_Position = u_Projection * u_View * worldPosition;
}