#version 460 core

layout(binding = 0) uniform sampler2D u_InputTexture1;
layout(binding = 1) uniform sampler2D u_InputTexture2;

in vec2 v_TexCoords;

out vec4 FragColor;

void main() 
{
    vec4 colA = texture(u_InputTexture1, v_TexCoords);
    vec4 colB = texture(u_InputTexture2, v_TexCoords);

    if (colA.r == 0.0 && colA.g == 0.0 && colA.b == 0.0)
        FragColor = vec4(colB.rgb, 1.0);
    else
        FragColor = vec4(colB.rgb * colA.a,1);
}