#version 400 core

// Inputs from the Vertex Shader
in vec3 vFragPos;      // Position in world space
in vec3 vNormal;       // Normal in world space
in vec2 vTexCoord;     // UV coordinates

// Uniforms
uniform sampler2D uTexture;   // Texture sampler
uniform vec3 uLightPos;       // Light position in world space
uniform vec3 uLightColor;     // Light color
uniform vec3 uViewPos;        // Camera position in world space

// Output color
out vec4 FragColor;

void main()
{
    FragColor = vec4(gl_PrimitiveID % 2, gl_PrimitiveID % 3, 1.0, 1.0);
    //FragColor = vec4(1.0, 1.0, 1.0, 1.0);
    /*
    // Normalize the input normal
    vec3 normal = normalize(vNormal);

    // Calculate the light direction
    vec3 lightDir = normalize(uLightPos - vFragPos);

    // Diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * uLightColor;

    // Ambient lighting (low intensity base light)
    vec3 ambient = 0.1 * uLightColor;

    // Combine lighting
    vec3 lighting = ambient + diffuse;

    // Sample the texture
    vec4 texColor = texture(uTexture, vTexCoord);

    // Final color output
    FragColor = vec4(lighting, 1.0) * texColor;
    */
}