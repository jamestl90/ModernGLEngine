#version 460 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;   
layout(location = 2) in vec2 a_TexCoord; 
layout(location = 3) in vec3 a_Tangent;  
layout(location = 4) in ivec4 a_Joints;
layout(location = 5) in vec4 a_Weights;

struct SkinnedMeshPerDrawData 
{
    mat4 modelMatrix;
    uint materialIndex;
    uint baseJointIndex;
};

layout(std430, binding = 0) readonly buffer SkinnedMeshPerDrawDataBuffer 
{
    SkinnedMeshPerDrawData perDrawData[];
};

layout(std430, binding = 1) readonly buffer GlobalTransforms 
{
    mat4 globalTransforms[];
};

uniform mat4 u_LightSpaceMatrix;

void main() 
{
    SkinnedMeshPerDrawData data = perDrawData[gl_DrawID + gl_InstanceID];
    mat4 modelMatrix = data.modelMatrix;

    float weightSum = a_Weights.x + a_Weights.y + a_Weights.z + a_Weights.w;
    vec4 normalizedWeights = a_Weights / weightSum;

    mat4 skinningMatrix =
        normalizedWeights.x * globalTransforms[data.baseJointIndex + a_Joints.x] +
        normalizedWeights.y * globalTransforms[data.baseJointIndex + a_Joints.y] +
        normalizedWeights.z * globalTransforms[data.baseJointIndex + a_Joints.z] +
        normalizedWeights.w * globalTransforms[data.baseJointIndex + a_Joints.w];

    vec4 worldPosition = skinningMatrix * vec4(a_Position, 1.0);

    gl_Position = u_LightSpaceMatrix * modelMatrix * worldPosition;
}