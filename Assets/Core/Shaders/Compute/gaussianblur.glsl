#version 460

layout(local_size_x = 16, local_size_y = 16) in;

layout(binding = 0, rgba8) uniform image2D u_InputImage;
layout(binding = 1, rgba8) uniform image2D u_OutputImage;

// Uniforms for blur
uniform int u_Radius;
uniform vec2 u_TexelSize; // (1.0 / width, 1.0 / height)

void main()
{
    ivec2 gid = ivec2(gl_GlobalInvocationID.xy); // Get global thread ID
    vec4 color = vec4(0.0);
    float totalWeight = 0.0;

    // Apply Gaussian blur kernel
    for (int i = -u_Radius; i <= u_Radius; ++i)
    {
        vec2 offset = vec2(i) * u_TexelSize; // Dynamic offset based on u_TexelSize
        float weight = exp(-float(i * i) / float(u_Radius * u_Radius)); // Gaussian weight
        color += imageLoad(u_InputImage, gid + ivec2(offset)) * weight;
        totalWeight += weight;
    }

    // Normalize the result
    color /= totalWeight;

    // Write the result to the output image
    imageStore(u_OutputImage, gid, color);
}