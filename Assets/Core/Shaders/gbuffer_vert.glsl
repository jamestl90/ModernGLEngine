#version 460 core

layout(location = 0) in vec3 a_Position;  // Vertex position
layout(location = 1) in vec3 a_Normal;    // Vertex normal
layout(location = 2) in vec2 a_TexCoord; // Texture coordinates
layout(location = 3) in vec3 a_Tangent;   // Tangent for normal mapping

struct PerDrawData {
    mat4 modelMatrix;
    int materialID;
    int padding[3]; // Padding to align the struct to 16 bytes
};

layout(std430, binding = 2) buffer PerDrawDataBuffer {
    PerDrawData perDrawData[];
};

uniform mat4 u_View;
uniform mat4 u_Projection;

out vec3 v_Normal;
out vec2 v_TexCoord;
out vec3 v_Tangent;
out vec3 v_Bitangent;
flat out int v_MaterialID;

void main() 
{
    // Fetch per-draw data
    PerDrawData data = perDrawData[gl_BaseInstance];
    mat4 modelMatrix = data.modelMatrix;
    v_MaterialID = data.materialID;

    vec4 worldPosition = modelMatrix * vec4(a_Position, 1.0);

    // Compute the normal matrix (transpose of the inverse of the upper-left 3x3 of modelMatrix)
    mat3 normalMatrix = mat3(transpose(inverse(modelMatrix)));

    // Transform attributes
    v_Normal = normalize(normalMatrix * a_Normal);
    v_Tangent = normalize(normalMatrix * a_Tangent);
    v_Bitangent = normalize(cross(v_Normal, v_Tangent)); // Compute bitangent

    // Pass through texture coordinates
    v_TexCoord = a_TexCoord;

    // Output final clip-space position
    gl_Position = u_Projection * u_View * worldPosition;
}