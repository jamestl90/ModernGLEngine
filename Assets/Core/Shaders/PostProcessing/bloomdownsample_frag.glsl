#version 460 core

in vec2 v_TexCoords;
out vec4 FragColor;

layout(binding = 0) uniform sampler2D u_HighResTexture;

void main() 
{
    vec2 texelSize = 1.0 / textureSize(u_HighResTexture, 0);

    vec3 result = vec3(0.0);

    result += texture(u_HighResTexture, v_TexCoords + vec2(-0.5, -0.5) * texelSize).rgb;
    result += texture(u_HighResTexture, v_TexCoords + vec2( 0.5, -0.5) * texelSize).rgb;
    result += texture(u_HighResTexture, v_TexCoords + vec2(-0.5,  0.5) * texelSize).rgb;
    result += texture(u_HighResTexture, v_TexCoords + vec2( 0.5,  0.5) * texelSize).rgb;
    result *= 0.25;

    //result.x = isnan(result.x) ? 0.0 : result.x;
    //result.y = isnan(result.y) ? 0.0 : result.y;
    //result.z = isnan(result.z) ? 0.0 : result.z;

    FragColor = vec4(result, 1.0);
}