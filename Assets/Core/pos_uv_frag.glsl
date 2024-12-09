#version 460 core

in vec2 v_TexCoords;

layout(binding = 0) uniform sampler2D debugTexture;

out vec4 FragColor;

void main() 
{
    vec3 tex = texture(debugTexture, v_TexCoords).rgb;

    FragColor = vec4(tex, 1.0);
}
