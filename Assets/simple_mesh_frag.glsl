#version 400 core

// Inputs from the Vertex Shader
in vec3 vFragPos;      // Position in world space
in vec3 vNormal;       // Normal in world space
in vec2 vTexCoord;     // UV coordinates

// Uniforms
uniform sampler2D uTexture;   // Texture sampler
uniform vec3 uLightPos;       // Light position in world space
uniform vec3 uLightColor;     // Light color

// Output color
out vec4 FragColor;

void main()
{
    vec3 normal = normalize(vNormal);
    vec3 lightDir = normalize(uLightPos - vFragPos);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * uLightColor;
    vec3 ambient = 0.1 * uLightColor;
    vec3 lighting = ambient + diffuse;
    vec4 texColor = texture(uTexture, vTexCoord);
    FragColor = vec4(lighting, 1.0) * texColor;
}