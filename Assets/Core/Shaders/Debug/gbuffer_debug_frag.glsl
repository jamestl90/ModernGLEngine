#version 460 core

in vec2 v_TexCoords;

uniform sampler2D gAlbedoAO;
uniform sampler2D gNormals;
uniform sampler2D gMetallicRoughness;
uniform sampler2D gEmissive;
uniform sampler2D gDepth;           // Depth buffer sampler
uniform int debugMode;            // 0 = Albedo, 1 = Normals, 2 = Metallic, 3 = AO, 4 = Roughness, 5 = Emissive

uniform float u_Near;
uniform float u_Far;

out vec4 FragColor;

bool decodeReceiveShadows(float encodedValue) 
{
    return fract(encodedValue) > 0.05; // Fractional part determines receive shadows
}

bool decodeCastShadows(float encodedValue) 
{
    return floor(encodedValue) > 0.5; // Integer part determines cast shadows
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
        vec4 normalSample = texture(gNormals, v_TexCoords);
        FragColor = vec4(normalSample.rgb * 0.5 + 0.5, 1.0);
        //bool receiveShadows = decodeReceiveShadows(normalSample.w);
        //bool castShadows = decodeCastShadows(normalSample.w);
        //FragColor = vec4(castShadows ? 1.0 : 0.0, 0.0, 0, 1);
    } 
    else if (debugMode == 2) 
    {
        // Metallic (R channel of gMetallicRoughness)
        vec4 mr = texture(gMetallicRoughness, v_TexCoords);
        FragColor = vec4(0.0, mr.g, mr.b, 1.0);
    } 
    else if (debugMode == 3) 
    {
        // Ambient Occlusion (A channel of gAlbedoAO)
        FragColor = vec4(vec3(texture(gAlbedoAO, v_TexCoords).a), 1.0);
    } 
    else if (debugMode == 4) 
    {
        float depth = texture(gDepth, v_TexCoords).r;
        FragColor = vec4(vec3(depth), 1.0); // Normalize to [0, 1]
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
