#version 450 core

layout(location = 0) in vec3 a_Position;  // Cube vertex position

uniform mat4 u_Projection;  // Projection matrix (perspective for cubemap rendering)
uniform mat4 u_View;        // View matrix for the current cubemap face

out vec3 v_Direction;       // Direction vector to pass to the fragment shader

void main()
{
    // Transform the vertex position to clip space
    gl_Position = u_Projection * u_View * vec4(a_Position, 1.0);

    // Compute the direction vector in world space
    v_Direction = mat3(u_View) * a_Position;  // Remove translation from the view matrix
}