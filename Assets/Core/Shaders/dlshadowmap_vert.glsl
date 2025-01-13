#version 460 core

layout(location = 0) in vec3 a_Position;

struct PerDrawData 
{
    mat4 modelMatrix;
    uint materialIndex;
};

layout(std430, binding = 0) readonly buffer PerDrawDataBuffer 
{
    PerDrawData perDrawData[];
};

uniform mat4 u_LightSpaceMatrix;

void main() 
{
    PerDrawData data = perDrawData[gl_DrawID + gl_InstanceID];
    mat4 modelMatrix = data.modelMatrix;

    gl_Position = u_LightSpaceMatrix * modelMatrix * vec4(a_Position, 1.0);
}