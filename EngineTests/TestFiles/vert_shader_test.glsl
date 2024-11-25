#version 400
layout (location = 0) in vec3 in_Position;
uniform mat4 u_MVP;
// out vec3 Colour;

void main()
{
    gl_Position = u_MVP * vec4(in_Position, 1.0);
    // Colour = in_Colour;
}