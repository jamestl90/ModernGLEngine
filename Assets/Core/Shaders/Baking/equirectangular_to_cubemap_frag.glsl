#version 460 core

out vec4 FragColor;
in vec3 v_Position;

uniform sampler2D u_EquirectangularMap;

uniform float u_CompressionThreshold;
uniform float u_MaxValue;

const vec2 invAtan = vec2(0.1591, 0.3183);

vec2 SampleSphericalMap(vec3 v)
{
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= invAtan;
    uv += 0.5;
    return uv;
}

void main()
{
    vec2 uv = SampleSphericalMap(normalize(v_Position));
    vec3 color = texture(u_EquirectangularMap, uv).rgb;

    float threshold = max(u_CompressionThreshold, 0.001); 
    color = color / (1.0 + color / threshold);
    color = min(color, vec3(u_MaxValue));
    
    FragColor = vec4(color, 1.0);
}
