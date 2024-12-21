#version 460 core

layout(location = 0) in vec2 a_Position;
layout(location = 1) in vec2 a_TexCoords;

out vec2 v_TexCoords;

void main() 
{
    v_TexCoords = a_TexCoords;
    v_TexCoords = vec2(a_TexCoords.x, 1.0 - a_TexCoords.y);
    gl_Position = vec4(a_Position, 0.0, 1.0);
}