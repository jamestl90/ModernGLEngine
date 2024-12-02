#version 460 core

layout(location = 0) in vec3 a_Position;  // Vertex position
layout(location = 1) in vec3 a_Normal;    // Vertex normal
layout(location = 2) in vec2 a_TexCoords; // Texture coordinates
layout(location = 3) in vec3 a_Tangent;   // Tangent for normal mapping

// Outputs to fragment shader
out vec3 v_FragPos;       // Fragment position in world space
out vec3 v_Normal;        // Normal in world space
out vec2 v_TexCoords;     // Texture coordinates
out vec3 v_Tangent;       // Tangent in world space
out vec3 v_Bitangent;     // Bitangent in world space

// Uniforms
uniform mat4 u_Model;     // Model matrix
uniform mat4 u_View;      // View matrix
uniform mat4 u_Projection;// Projection matrix

void main() {
    // Calculate world-space positions and normals
    v_FragPos = vec3(u_Model * vec4(a_Position, 1.0));
    v_Normal = mat3(transpose(inverse(u_Model))) * a_Normal; // Transform normal to world space

    // Pass texture coordinates
    v_TexCoords = a_TexCoords;

    // Calculate tangent and bitangent for normal mapping
    v_Tangent = normalize(mat3(u_Model) * a_Tangent);
    v_Bitangent = cross(v_Normal, v_Tangent);

    // Output clip space position
    gl_Position = u_Projection * u_View * vec4(v_FragPos, 1.0);
}