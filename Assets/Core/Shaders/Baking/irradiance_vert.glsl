#version 460 core

layout(location = 0) in vec3 aPos;

out vec3 WorldPos;

uniform mat4 u_Projection;
uniform mat4 u_View;

void main()
{
    WorldPos = aPos;  // Pass the position in world space to the fragment shader
    gl_Position = u_Projection * u_View * vec4(WorldPos, 1.0);
}
