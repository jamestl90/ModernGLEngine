#version 460 core

in vec2 v_TexCoords;

uniform sampler2D gAlbedoAO;
uniform sampler2D gNormals;
uniform sampler2D gMetallicRoughness;
uniform sampler2D gEmissive;
uniform sampler2D gDepth; // Depth buffer sampler
uniform int debugMode;            // 0 = Albedo, 1 = Normals, 2 = Metallic, 3 = AO, 4 = Roughness, 5 = Emissive

out vec4 FragColor;

float LinearizeDepth(float depth, float farPlane, float nearPlane) 
{
    float z = depth * 2.0 - 1.0; // Convert [0,1] depth to NDC [-1,1]
    return (2.0 * nearPlane * farPlane) / (farPlane + nearPlane - z * (farPlane - nearPlane));
}

void main() 
{
    if (debugMode == 0) 
    {
        // Albedo (RGB from gAlbedoAO)
        FragColor = vec4(texture(gAlbedoAO, v_TexCoords).rgb, 1.0);
    } 
    else if (debugMode == 1) 
    {
        // Normals (mapped from [-1, 1] to [0, 1])
        FragColor = vec4(texture(gNormals, v_TexCoords).rgb * 0.5 + 0.5, 1.0);
    } 
    else if (debugMode == 2) 
    {
        // Metallic (R channel of gMetallicRoughness)
        FragColor = vec4(vec2(texture(gMetallicRoughness, v_TexCoords).rg), 0.0, 1.0);
    } 
    else if (debugMode == 3) 
    {
        // Ambient Occlusion (A channel of gAlbedoAO)
        FragColor = vec4(vec3(texture(gAlbedoAO, v_TexCoords).a), 1.0);
    } 
    else if (debugMode == 4) 
    {
        float nearPlane = 0.1f;
        float farPlane = 10.0f;
        float depth = texture(gDepth, v_TexCoords).r;
        float linearDepth = LinearizeDepth(depth, farPlane, nearPlane);
        FragColor = vec4(vec3(linearDepth / farPlane), 1.0); // Normalize to [0, 1]
    } 
    else if (debugMode == 5) 
    {
        // Emissive (RGB from gEmissive)
        FragColor = texture(gEmissive, v_TexCoords);
    } 
    else 
    {
        // Default (black if mode is invalid)
        FragColor = vec4(0.0, 0.0, 0.0, 1.0);
    }
}
