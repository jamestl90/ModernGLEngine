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

layout(std430, binding = 1) readonly buffer SkinnedMeshPerDrawDataBuffer 
{
    SkinnedMeshPerDrawData perDrawData[];
};

layout(std140, binding = 2) uniform CameraInfo 
{
    mat4 viewMatrix;
    mat4 projMatrix;
    vec4 cameraPosition;
    vec4 timeInfo;
};

layout(std430, binding = 3) readonly buffer GlobalTransforms 
{
    mat4 globalTransforms[];
};

out vec3 v_Normal;
out vec2 v_TexCoord;
out vec3 v_Tangent;
out vec3 v_Bitangent;
flat out uint v_MaterialIndex;

void main() 
{
    SkinnedMeshPerDrawData data = perDrawData[gl_DrawID + gl_InstanceID];
    mat4 modelMatrix = data.modelMatrix;
    v_MaterialIndex = data.materialIndex;

    v_TexCoord = a_TexCoord;

    float weightSum = a_Weights.x + a_Weights.y + a_Weights.z + a_Weights.w;
    vec4 normalizedWeights = a_Weights / weightSum;

    // debug my weight values, model should only show if the weights are ~1.0
    if (weightSum < 0.99 || weightSum > 1.01)
    {
        gl_Position = vec4(0.0);
        return;
    }

    mat4 skinningMatrix =
        normalizedWeights.x * globalTransforms[data.baseJointIndex + a_Joints.x] +
        normalizedWeights.y * globalTransforms[data.baseJointIndex + a_Joints.y] +
        normalizedWeights.z * globalTransforms[data.baseJointIndex + a_Joints.z] +
        normalizedWeights.w * globalTransforms[data.baseJointIndex + a_Joints.w];

    vec4 worldPosition = skinningMatrix * vec4(a_Position, 1.0);

    mat3 modelMatrixNormal = transpose(inverse(mat3(modelMatrix)));
    mat3 skinningMatrixNormal = transpose(inverse(mat3(skinningMatrix)));
    v_Normal = normalize(modelMatrixNormal * (skinningMatrixNormal * a_Normal));
    v_Tangent = normalize(modelMatrixNormal * (skinningMatrixNormal * a_Tangent));
    v_Bitangent = normalize(cross(v_Normal, v_Tangent)); 

    mat4 mvp = projMatrix * viewMatrix * modelMatrix;

    // clip-space position
    gl_Position = mvp * worldPosition;
}