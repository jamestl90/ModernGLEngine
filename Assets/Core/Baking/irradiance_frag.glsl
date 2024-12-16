#version 330 core
out vec4 FragColor;

in vec3 WorldPos;

uniform samplerCube u_EnvironmentMap;
const float PI = 3.14159265359;

// Tangent space vectors
uniform vec3 u_TangentSamples[16]; // Precomputed sample directions

void main()
{
    // Normalize the direction to the current texel
    vec3 normal = normalize(WorldPos);

    // Integrate over the hemisphere
    vec3 irradiance = vec3(0.0);
    for (int i = 0; i < 16; ++i)
    {
        // Convert tangent space sample to world space
        vec3 tangentSample = u_TangentSamples[i];
        vec3 sampleVec = tangentSample.x * cross(normal, vec3(0.0, 1.0, 0.0)) +
                         tangentSample.y * cross(normal, vec3(0.0, 0.0, 1.0)) +
                         tangentSample.z * normal;

        // Fetch environment map texel in the sample direction
        irradiance += texture(u_EnvironmentMap, sampleVec).rgb * max(dot(normal, sampleVec), 0.0);
    }

    irradiance = irradiance * (PI / float(16)); // Divide by sample count and multiply by PI
    FragColor = vec4(irradiance, 1.0);
}
