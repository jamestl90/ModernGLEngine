#ifndef ATMOSPHERE_COMMON_GLSL
#define ATMOSPHERE_COMMON_GLSL

#define PI 3.14159265358979323846

// Matches C++ struct AtmosphereParams with std140 layout.
// vec3 members have a base alignment of 16 bytes.
layout(std140, binding = 0) uniform AtmosphereUBO {
    // Sun
    vec3 solarIrradiance;           // offset 0
    float sunAngularRadius;         // offset 12
    vec3 sunDir;                    // offset 16
    float exposure;                 // offset 28

    // Earth
    float bottomRadiusKM;           // offset 32
    float topRadiusKM;              // offset 36
                                    // implicit padding to 48 for next vec3 (groundAlbedo)
    vec3 groundAlbedo;              // offset 48
                                    // implicit padding to 64 for next vec3 (rayleighScatteringCoeff)
    // Rayleigh Scattering
    vec3 rayleighScatteringCoeff;   // offset 64
    float rayleighDensityHeightScale; // offset 76

    // Mie Scattering
    vec3 mieScatteringCoeff;        // offset 80 (next multiple of 16 after 76+4=80)
                                    // implicit padding to 96 for next vec3 (mieExtinctionCoeff)
    vec3 mieExtinctionCoeff;        // offset 96
    float mieDensityHeightScale;    // offset 108
    float miePhaseG;                // offset 112
                                    // implicit padding to 128 for next vec3 (absorptionExtinction)
    // Absorption
    vec3 absorptionExtinction;      // offset 128
    float absorptionDensityHeightScale; // offset 140
} params;


// Density functions (h: height above surface in km)
float GetRayleighDensity(float h) 
{
    return exp(-h / params.rayleighDensityHeightScale);
}

float GetMieDensity(float h) 
{
    return exp(-h / params.mieDensityHeightScale);
}

float GetAbsorptionDensity(float h) 
{
    const float ozone_center_altitude_km = 25.0f;
    float dh = h - ozone_center_altitude_km;
    return exp(-abs(dh) / params.absorptionDensityHeightScale);
}

vec3 GetExtinctionCoeff(float h_km) 
{
    float h = max(0.0, h_km); 
    return params.rayleighScatteringCoeff * GetRayleighDensity(h) +
           params.mieExtinctionCoeff * GetMieDensity(h) +
           params.absorptionExtinction * GetAbsorptionDensity(h);
}

vec3 GetScatteringCoeffRayleigh(float h_km) 
{
    float h = max(0.0, h_km);
    return params.rayleighScatteringCoeff * GetRayleighDensity(h);
}

vec3 GetScatteringCoeffMie(float h_km) 
{
    float h = max(0.0, h_km);
    return params.mieScatteringCoeff * GetMieDensity(h);
}

// Phase functions
float RayleighPhase(float cosTheta) 
{
    return (3.0 / (16.0 * PI)) * (1.0 + cosTheta * cosTheta);
}

float HenyeyGreensteinPhase(float cosTheta, float g) 
{
    float g2 = g * g;
    float denom = 1.0 + g2 - 2.0 * g * cosTheta;
    return (1.0 / (4.0 * PI)) * (1.0 - g2) / (denom * sqrt(denom));
}

// Ray-sphere intersection
vec2 IntersectSphere(vec3 o, vec3 d, float r) 
{
    float r2 = r * r;
    float b = dot(o, d);
    float c = dot(o, o) - r2;
    float delta = b * b - c;
    if (delta < 0.0) return vec2(-1.0, -1.0);
    float sqrtDelta = sqrt(delta);
    return vec2(-b - sqrtDelta, -b + sqrtDelta);
}

// Computes optical depth along a ray segment from p1 to p2.
vec3 OpticalDepth(vec3 p1_world, vec3 p2_world, int num_steps) 
{
    vec3 totalOpticalDepth = vec3(0.0);
    vec3 dir_segment = p2_world - p1_world;
    float segmentLength = length(dir_segment);
    if (segmentLength < 1e-4) return vec3(0.0);
    dir_segment /= segmentLength;

    float stepSize = segmentLength / float(num_steps);

    for (int i = 0; i < num_steps; ++i) {
        vec3 current_pos_world = p1_world + dir_segment * (float(i) + 0.5) * stepSize;
        float h = length(current_pos_world) - params.bottomRadiusKM;
        
        if (h >= 0.0 && h <= (params.topRadiusKM - params.bottomRadiusKM)) {
             totalOpticalDepth += GetExtinctionCoeff(h) * stepSize;
        }
    }
    return totalOpticalDepth;
}

vec2 AltitudeCosAngleToUV(float altitude_km, float cos_angle_from_zenith, float atmosphere_thickness_km) 
{
    // Ensure altitude is clamped for safe division and UV range
    float clamped_alt_km = clamp(altitude_km, 0.0, atmosphere_thickness_km);
    // Avoid division by zero if thickness is zero or negative
    float u = (atmosphere_thickness_km > 1e-4) ? (clamped_alt_km / atmosphere_thickness_km) : 0.0;
    float v = cos_angle_from_zenith * 0.5 + 0.5; // Map [-1,1] to [0,1]
    return vec2(clamp(u, 0.0, 1.0), clamp(v, 0.0, 1.0));
}

vec2 UVToAltitudeCosAngle(vec2 uv, float atmosphere_thickness_km) 
{
    float altitude_km = uv.x * atmosphere_thickness_km;
    float cos_angle_from_zenith = uv.y * 2.0 - 1.0; // Map [0,1] to [-1,1]
    return vec2(altitude_km, cos_angle_from_zenith);
}

#endif // ATMOSPHERE_COMMON_GLSL