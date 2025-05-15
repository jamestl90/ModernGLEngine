#version 460 core

out vec4 FragColor;
in vec3 v_Position;

uniform float u_Roughness;
uniform int u_NumSamples;

layout (binding = 0) uniform samplerCube u_EnvironmentMap;

const float PI = 3.14159265359;

vec2 Hammersley(uint i, uint N)
{
    uint bits = (i << 16u) | (i >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    float radicalInverseVDC = float(bits) * 2.3283064365386963e-10; // / 0x100000000 or / 2^32
    return vec2(float(i) / float(N), radicalInverseVDC);
}

vec3 ImportanceSampleGGX(vec2 Xi, vec3 N, float roughness)
{
	float a = roughness*roughness;
	
	float phi = 2.0 * PI * Xi.x;
	float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a*a - 1.0) * Xi.y));
	float sinTheta = sqrt(1.0 - cosTheta*cosTheta);
	
	// from spherical coordinates to cartesian coordinates - halfway vector
	vec3 H;
	H.x = cos(phi) * sinTheta;
	H.y = sin(phi) * sinTheta;
	H.z = cosTheta;
	
	// from tangent-space H vector to world-space sample vector
	vec3 up        = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
	vec3 tangent   = normalize(cross(up, N));
	vec3 bitangent = cross(N, tangent);
	
	vec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
	return normalize(sampleVec);
}

void main()
{
    vec3 N = normalize(v_Position);
    vec3 R = N;
    vec3 V = R;

    vec3 prefilteredColor = vec3(0.0);
    float totalWeight = 0.0;

    for (int i = 0; i < u_NumSamples; ++i) 
    {
        //vec2 Xi = vec2(float(i) / float(u_NumSamples), fract(float(i) * 0.61803398875));
        vec2 Xi = Hammersley(uint(i), uint(u_NumSamples));
        vec3 H = ImportanceSampleGGX(Xi, N, u_Roughness);
        vec3 L = normalize(2.0 * dot(V, H) * H - V);

        float NdotL = max(dot(N, L), 0.0);
        if (NdotL > 0.0) 
        {
            prefilteredColor += texture(u_EnvironmentMap, L).rgb * NdotL;
            totalWeight += NdotL;
        }
    }

    prefilteredColor = (totalWeight > 0.0) ? (prefilteredColor / totalWeight) : vec3(0.0);
    FragColor = vec4(prefilteredColor, 1.0);
}