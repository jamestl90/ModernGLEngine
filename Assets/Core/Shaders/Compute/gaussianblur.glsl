#version 460

const int LOCAL_SIZE_X = 16;
const int LOCAL_SIZE_Y = 16;

layout(local_size_x = LOCAL_SIZE_X, local_size_y = LOCAL_SIZE_Y) in;

layout(binding = 0, rgba8) uniform image2D u_InputImage;

uniform bool u_Horizontal;

const int radius = 4;
const float blurWeights[9] = float[9](0.0033, 0.0238, 0.0971, 0.2259, 0.2993, 0.2259, 0.0971, 0.0238, 0.0033);

shared vec4 sharedTile[LOCAL_SIZE_X  + 2 * radius][LOCAL_SIZE_Y + 2 * radius];

void main()
{
    ivec2 gid = ivec2(gl_GlobalInvocationID.xy);      // Global thread ID
    ivec2 lid = ivec2(gl_LocalInvocationID.xy);       // Local thread ID
    ivec2 groupSize = ivec2(gl_WorkGroupSize.xy);     // Workgroup size
    ivec2 tileSize = groupSize + ivec2(2 * radius);   // Shared memory tile size

    ivec2 sharedCoords = lid + ivec2(radius);
    ivec2 imgSize = imageSize(u_InputImage);

    // Load data into shared memory with halo regions
    for (int y = -radius; y <= radius; y++)
    {
        for (int x = -radius; x <= radius; x++)
        {
            ivec2 offset = ivec2(x, y);
            ivec2 globalCoords = clamp(gid + offset, ivec2(0), imgSize - 1);
            sharedTile[sharedCoords.x + x][sharedCoords.y + y] = imageLoad(u_InputImage, globalCoords);
        }
    }

    barrier();

    ivec2 direction = ivec2(u_Horizontal ? 1 : 0, u_Horizontal ? 0 : 1);
    vec4 color = vec4(0.0);
    // Apply Gaussian blur kernel
    for (int i = -radius; i <= radius; i++)
    {
        ivec2 offset = direction * i;
        ivec2 sharedSampleCoords = clamp(sharedCoords + offset, ivec2(0), tileSize - 1);
        color += sharedTile[sharedSampleCoords.x][sharedSampleCoords.y] * blurWeights[i + radius];
    }

    // Write the result to the output image
    if (gid.x < imgSize.x && gid.y < imgSize.y)
    {
        imageStore(u_InputImage, gid, color);
    }
}