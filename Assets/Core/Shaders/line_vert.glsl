#version 460 core
layout(location = 0) in vec3 a_Position;

uniform mat4 u_ViewProjectionMatrix;

void main()
{
    gl_Position = u_ViewProjectionMatrix * vec4(a_Position, 1.0);
}