#version 450 core

uniform samplerCube u_CubeMap; // The source cubemap texture
uniform float u_SrcMipLevel;  // The source mipmap level (higher resolution)

in vec3 v_Direction;          // Incoming direction from the vertex shader
out vec4 FragColor;           // The downsampled color output

void main()
{
    // Sample the current cubemap mip level
    vec3 color = textureLod(u_CubeMap, v_Direction, u_SrcMipLevel).rgb;

    // Perform additional filtering if necessary (e.g., Gaussian filtering)
    FragColor = vec4(color, 1.0);
}