#version 460 core

layout(binding = 0) uniform sampler2D DirectLight;
layout(binding = 1) uniform sampler2D IBL;
layout(binding = 2) uniform sampler2D gEmissive;
layout(binding = 3) uniform sampler2D gAlbedo;

layout(location = 0) out vec4 FragColor;

layout(std140, binding = 0) uniform ShaderGlobalData 
{
    mat4 viewMatrix;
    mat4 projMatrix;
    vec4 camPos;
    vec4 camDir;
    vec2 timeInfo;
    vec2 windowSize;
    int frameCount;
};

in vec2 v_TexCoords;

void main()
{
    //vec3 rayMarchColor = texture(rayMarchTex, v_TexCoords).rgb;
    vec3 directLighting = texture(DirectLight, v_TexCoords).rgb;
    vec3 iblLighting = texture(IBL, v_TexCoords).rgb;
    vec4 albedo = texture(gAlbedo, v_TexCoords);
    float ao = albedo.a;
    vec3 emissive = texture(gEmissive, v_TexCoords).rgb;

    //vec3 totalLighting = directLighting + iblLighting + (rayMarchColor * 0.3);
    vec3 totalLighting = directLighting + iblLighting;

    totalLighting += emissive; 

    if (ao > 0.0)
        totalLighting *= ao;

    FragColor = vec4(totalLighting, 1.0);
}