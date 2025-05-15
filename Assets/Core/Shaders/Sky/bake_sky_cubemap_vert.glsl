#version 460 core

layout(location = 0) in vec3 a_Position;

uniform mat4 u_Projection;
uniform mat4 u_View;

out VS_OUT 
{
    vec3 viewDir_world; // world-space view direction for this fragment
} vs_out;

void main()
{
    vs_out.viewDir_world = a_Position;
    vec4 clipPos = u_Projection * u_View * vec4(a_Position, 1.0);
    gl_Position = clipPos.xyww;
}
