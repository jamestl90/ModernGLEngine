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

layout(std140, binding = 2) uniform CameraInfo 
{
    mat4 viewMatrix;
    mat4 projMatrix;
    vec3 cameraPosition;
};

out vec3 v_Normal;
out vec2 v_TexCoord;
out vec2 v_ScreenCoord;
flat out uint v_MaterialIndex;

void main() 
{
    PerDrawData data = perDrawData[gl_DrawID];
    mat4 modelMatrix = data.modelMatrix;
    v_MaterialIndex = data.materialIndex;

    mat3 normalMatrix = mat3(transpose(inverse(modelMatrix)));

    v_Normal = normalize(normalMatrix * a_Normal);

    v_TexCoord = a_TexCoord;

    // clip-space position
    vec4 worldPosition = modelMatrix * vec4(a_Position, 1.0);
    gl_Position = projMatrix * viewMatrix * worldPosition;

    v_ScreenCoord = gl_Position.xy / gl_Position.w * 0.5 + 0.5;
}