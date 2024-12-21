#version 460 core

in vec2 v_TexCoords;

layout(binding = 0) uniform sampler2D u_Texture;

out vec4 FragColor;

void main() 
{
    vec3 tex = texture(u_Texture, v_TexCoords).rgb;
    FragColor = vec4(tex, 1.0);
}
