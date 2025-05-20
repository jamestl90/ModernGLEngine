#version 460 core

layout(binding = 0) uniform sampler2D DirectLight;
layout(binding = 1) uniform sampler2D SpecularIBL;
layout(binding = 2) uniform sampler2D IndirectLight; // ddgi

layout(location = 0) out vec4 FragColor;

in vec2 v_TexCoords;

uniform bool u_NeedsGammaCorrection;
uniform float u_Exposure;

vec3 ToneMapLinear(vec3 hdrColor, float exposure) 
{
    vec3 exposedColor = hdrColor * exposure;
    return clamp(exposedColor, 0.0, 1.0); // Harsh clamp
}

vec3 ToneMapReinhardSimple(vec3 hdrColor, float exposure) 
{
    vec3 exposedColor = hdrColor * exposure;
    return exposedColor / (exposedColor + vec3(1.0));
}

vec3 ToneMapReinhardExtended(vec3 hdrColor, float exposure, float whitePoint) 
{
    vec3 exposedColor = hdrColor * exposure;
    float whitePointSquared = whitePoint * whitePoint;
    vec3 numerator = exposedColor * (vec3(1.0) + exposedColor / vec3(whitePointSquared));
    return numerator / (vec3(1.0) + exposedColor);
}

vec3 Uncharted2TonemapPartial(vec3 x) 
{
    float A = 0.15; // Shoulder Strength
    float B = 0.50; // Linear Strength
    float C = 0.10; // Linear Angle
    float D = 0.20; // Toe Strength
    float E = 0.02; // Toe Numerator
    float F = 0.30; // Toe Denominator
    return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}

vec3 ToneMapUncharted2(vec3 hdrColor, float exposure)
{
    float W = 11.2;
    vec3 exposedColor = hdrColor * exposure;
    vec3 curr = Uncharted2TonemapPartial(exposedColor);
    vec3 whiteScale = vec3(1.0) / Uncharted2TonemapPartial(vec3(W));
    vec3 ldrColor = curr * whiteScale;
    return ldrColor;
}

vec3 ToneMapACESFilmic(vec3 hdrColor, float exposure) 
{
    const float a = 2.51f;
    const float b = 0.03f;
    const float c = 2.43f;
    const float d = 0.59f;
    const float e = 0.14f;
    vec3 exposedColor = hdrColor * exposure;
    // (x * (a * x + b)) / (x * (c * x + d) + e)
    vec3 ldrColor = (exposedColor * (a * exposedColor + vec3(b))) / (exposedColor * (c * exposedColor + vec3(d)) + vec3(e));
    return clamp(ldrColor, 0.0, 1.0); 
}

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
    
    combinedLight = ToneMapACESFilmic(combinedLight, u_Exposure);

    if (u_NeedsGammaCorrection)
    {
        combinedLight = ApplyGammaCorrection(combinedLight);
    }

    FragColor = vec4(combinedLight, 1.0);
}