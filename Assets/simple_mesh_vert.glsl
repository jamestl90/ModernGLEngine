#version 400 core

// Input attributes from the Vertex Buffer
layout(location = 0) in vec3 aPosition;  // Vertex position
layout(location = 1) in vec3 aNormal;    // Vertex normal
layout(location = 2) in vec2 aTexCoord;  // Vertex UV coordinates

// Uniforms
uniform mat4 uModel;        // Model matrix
uniform mat4 uView;         // View matrix
uniform mat4 uProjection;   // Projection matrix

// Outputs to the Fragment Shader
out vec3 vFragPos;          // Position in world space
out vec3 vNormal;           // Normal in world space
out vec2 vTexCoord;         // UV coordinates

void main()
{
    // Transform the vertex position to clip space
    gl_Position = uProjection * uView * uModel * vec4(aPosition, 1.0);

    // Pass world-space position and normal to the fragment shader
    vFragPos = vec3(uModel * vec4(aPosition, 1.0));
    vNormal = mat3(transpose(inverse(uModel))) * aNormal;

    // Pass UV coordinates to the fragment shader
    vTexCoord = aTexCoord;
}