#version 460 core

layout(binding = 0) uniform sampler2D DirectLight;
layout(binding = 1) uniform sampler2D SpecularIBL;
layout(binding = 2) uniform sampler2D IndirectLight; // ddgi

layout(location = 0) out vec4 FragColor;

in vec2 v_TexCoords;

vec3 ApplyToneMapping(vec3 color)
{
    return color / (color + vec3(1.0));
}

vec3 ApplyGammaCorrection(vec3 color)
{
    return pow(color, vec3(1.0 / 2.2));
}

void main()
{
    vec3 directLighting = texture(DirectLight, v_TexCoords).rgb;
    vec3 specularIBLLighting = texture(SpecularIBL, v_TexCoords).rgb;
    vec3 indirectLighting = texture(IndirectLight, v_TexCoords).rgb;

    vec3 combinedLight = directLighting + specularIBLLighting + indirectLighting;
    
    // combinedLight = ApplyToneMapping(combinedLight);
    // combinedLight = ApplyGammaCorrection(combinedLight);

    FragColor = vec4(combinedLight, 1.0);
}