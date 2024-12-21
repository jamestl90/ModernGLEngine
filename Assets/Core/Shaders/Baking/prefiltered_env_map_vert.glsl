#version 460 core

layout(location = 0) in vec3 position;

out vec3 v_WorldPos;

uniform mat4 u_Projection;
uniform mat4 u_View;

void main() 
{
    v_WorldPos = position;
    gl_Position = u_Projection * u_View * vec4(position, 1.0);
}