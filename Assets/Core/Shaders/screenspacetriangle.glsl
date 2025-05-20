#version 460

vec4 clipSpacePositions[3] = vec4[](
    vec4(-1.0, -1.0, 0.0, 1.0), // Bottom-left corner
    vec4( 3.0, -1.0, 0.0, 1.0), // Bottom-right beyond right edge
    vec4(-1.0,  3.0, 0.0, 1.0)  // Top-left beyond top edge
);

vec2 texCoords[3] = vec2[](
    vec2(0.0, 0.0), // Bottom-left corner
    vec2(2.0, 0.0), // Bottom-right beyond right edge
    vec2(0.0, 2.0)  // Top-left beyond top edge
);

out vec2 v_TexCoords;

void main()
{
    gl_Position = clipSpacePositions[gl_VertexID];
    v_TexCoords = texCoords[gl_VertexID];
}