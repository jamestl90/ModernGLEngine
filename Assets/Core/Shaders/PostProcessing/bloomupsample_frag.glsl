#version 460 core

in vec2 v_TexCoords;

out vec4 FragColor;

layout(binding = 0) uniform sampler2D u_LowResTexture;

uniform float u_Scatter;

void main() 
{

    vec2 texelSize = 1.0 / textureSize(u_LowResTexture, 0);

    vec3 result = vec3(0.0);

    float offsetScale = u_Scatter; 

    float hlim = offsetScale * texelSize.x;
    float vlim = offsetScale * texelSize.y;

    result += texture(u_LowResTexture, v_TexCoords + vec2(-hlim, -vlim)).rgb;
    result += texture(u_LowResTexture, v_TexCoords + vec2( hlim, -vlim)).rgb;
    result += texture(u_LowResTexture, v_TexCoords + vec2(-hlim,  vlim)).rgb;
    result += texture(u_LowResTexture, v_TexCoords + vec2( hlim,  vlim)).rgb;
    result *= 0.25; 

    //result.x = isnan(result.x) ? 0.0 : result.x;
    //result.y = isnan(result.y) ? 0.0 : result.y;
    //result.z = isnan(result.z) ? 0.0 : result.z;

    FragColor = vec4(result, 1.0);
}