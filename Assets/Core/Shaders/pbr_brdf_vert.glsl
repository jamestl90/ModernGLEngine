#version 460 core

// Input attributes (from vertex buffer)
layout(location = 0) in vec3 aPosition;  // Vertex position in model space
layout(location = 1) in vec3 aNormal;    // Vertex normal in model space
layout(location = 2) in vec2 aTexCoords; // Texture coordinates

// Uniforms
uniform mat4 model;       // Model matrix
uniform mat4 view;        // View matrix
uniform mat4 projection;  // Projection matrix
uniform mat3 normalMatrix; // Normal matrix (transpose of the inverse of the model matrix)

// Outputs to the fragment shader
out vec3 FragPos;      // Fragment position in world space
out vec3 Normal;       // Normal vector in world space
out vec2 TexCoords;    // Texture coordinates

void main() {
    // Transform vertex position to world space and then to clip space
    vec4 worldPosition = model * vec4(aPosition, 1.0);
    gl_Position = projection * view * worldPosition;

    // Pass world position to the fragment shader
    FragPos = worldPosition.xyz;

    // Transform the normal to world space using the normal matrix
    Normal = normalize(normalMatrix * aNormal);

    // Pass the texture coordinates through
    TexCoords = aTexCoords;
}
