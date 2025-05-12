#version 460

const vec4 clipSpacePositions[3] = vec4[](
    vec4(-1.0, -1.0, 0.0, 1.0), // Bottom-left
    vec4( 3.0, -1.0, 0.0, 1.0), // Far-right
    vec4(-1.0,  3.0, 0.0, 1.0)  // Far-top
);

out VS_OUT 
{
    vec3 viewDir_world;
} vs_out;

uniform mat4 invViewMatrix;       // Inverse of camera's view matrix
uniform mat4 invProjMatrix; // Inverse of camera's projection matrix

void main() 
{
    vec4 current_clip_pos_xy = clipSpacePositions[gl_VertexID];
    
    // Output to clip space, ensuring it's at the far plane for correct depth testing
    // Using current_clip_pos_xy.xy, and setting z to be far, w to 1.
    gl_Position = vec4(current_clip_pos_xy.xy, 0.9999999, 1.0); 

    // reconstruct world space position on the far plane to get view direction
    vec4 farPlane_clip = vec4(current_clip_pos_xy.xy, 1.0, 1.0); // At far plane in clip space
    vec4 farPlane_view = invProjMatrix * farPlane_clip;
    farPlane_view /= farPlane_view.w; // Perspective divide
    vec4 farPlane_world_h = invViewMatrix * farPlane_view;
    vec3 farPlane_world = farPlane_world_h.xyz;

    vec3 cameraPos_world = invViewMatrix[3].xyz; // Camera position from invViewMatrix

    vs_out.viewDir_world = (farPlane_world - cameraPos_world);
}