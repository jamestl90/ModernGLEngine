#version 460 core

in vec2 v_TexCoords;

layout(binding = 0) uniform sampler2D debugTexture;

uniform bool u_Linearize;
uniform float u_Near;
uniform float u_Far;

out vec4 FragColor;

float LinearizeDepth(float depth, float near, float far) 
{
    float z = depth * 2.0 - 1.0; // Convert to NDC space [-1, 1]
    return (2.0 * near * far) / (far + near - z * (far - near));
}

void main() 
{
    float depth = 0;
    if (u_Linearize)
    {
        float texDepth = texture(debugTexture, v_TexCoords).r;
        depth = LinearizeDepth(texDepth, u_Near, u_Far) / u_Far;
    }
    else
    {
        depth = texture(debugTexture, v_TexCoords).r;
    }

    FragColor = vec4(vec3(depth), 1.0);
}
