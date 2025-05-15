#version 460 core

layout(location = 0) out vec4 FragColor;

in VS_OUT 
{
    vec3 viewDir_world; 
} fs_in;

#include "common.glsl"

uniform vec3 cameraPos_world_km; // world position 
layout(binding = 0) uniform sampler2D transmittanceLUT_sampler;

const int NUM_VIEW_RAY_SAMPLES_BAKE = 32; // Can use higher quality for baking
const int NUM_VIEW_PATH_OPTICAL_DEPTH_SAMPLES_BAKE = 16;

vec3 SampleTransmittanceLUT(float altitude_km, float view_zenith_mu) 
{
    float atmosphere_thickness_km = params.topRadiusKM - params.bottomRadiusKM;
    vec2 uv = AltitudeCosAngleToUV(altitude_km, view_zenith_mu, atmosphere_thickness_km);
    return texture(transmittanceLUT_sampler, uv).rgb;
}

vec3 IntegrateSingleScattering(vec3 ray_origin_world, vec3 ray_dir_world_norm, vec3 sun_dir_world_norm) 
{
    vec3 L_s = vec3(0.0); 
    vec2 t_atmosphere_intersect = IntersectSphere(ray_origin_world, ray_dir_world_norm, params.topRadiusKM);
    vec2 t_planet_intersect = IntersectSphere(ray_origin_world, ray_dir_world_norm, params.bottomRadiusKM);

    float ray_start_offset = 0.0; 
    float ray_end_dist;

    if (length(ray_origin_world) > params.topRadiusKM) 
    {
        if (t_atmosphere_intersect.x > 0.0 && t_atmosphere_intersect.y > t_atmosphere_intersect.x) 
        {
            ray_start_offset = t_atmosphere_intersect.x;
            ray_end_dist = t_atmosphere_intersect.y;
        } 
        else 
        { 
            return vec3(0.0); 
        }
    } 
    else 
    { 
        ray_start_offset = 0.0;
        ray_end_dist = t_atmosphere_intersect.y;
    }

    if (t_planet_intersect.x > ray_start_offset && t_planet_intersect.x < ray_end_dist) 
    {
         ray_end_dist = t_planet_intersect.x;
    }
    
    float segment_length = ray_end_dist - ray_start_offset;
    if (segment_length <= 1e-3f) return vec3(0.0);

    float step_size = segment_length / float(NUM_VIEW_RAY_SAMPLES_BAKE);

    for (int i = 0; i < NUM_VIEW_RAY_SAMPLES_BAKE; ++i) 
    {
        float t_sample = ray_start_offset + (float(i) + 0.5) * step_size;
        vec3 p_sample_world = ray_origin_world + ray_dir_world_norm * t_sample; 
        float p_sample_altitude_km = length(p_sample_world) - params.bottomRadiusKM;

        if (p_sample_altitude_km < 0.0 || p_sample_altitude_km > (params.topRadiusKM - params.bottomRadiusKM)) 
        {
            continue; 
        }
        
        vec3 transmittance_viewer_to_sample = exp(-OpticalDepth(ray_origin_world, p_sample_world, NUM_VIEW_PATH_OPTICAL_DEPTH_SAMPLES_BAKE));
        
        vec3 p_sample_up_dir = normalize(p_sample_world);
        float sun_zenith_mu_at_sample = dot(p_sample_up_dir, sun_dir_world_norm);
        vec3 transmittance_sun_to_sample = SampleTransmittanceLUT(p_sample_altitude_km, sun_zenith_mu_at_sample);
        
        float cos_scatter_angle = dot(ray_dir_world_norm, sun_dir_world_norm);
        vec3 rayleigh_contrib = GetScatteringCoeffRayleigh(p_sample_altitude_km) * RayleighPhase(cos_scatter_angle);
        vec3 mie_contrib = GetScatteringCoeffMie(p_sample_altitude_km) * HenyeyGreensteinPhase(cos_scatter_angle, params.miePhaseG);
        
        vec3 total_scattering_at_sample = rayleigh_contrib + mie_contrib;
        vec3 in_scattered_light = total_scattering_at_sample * params.solarIrradiance;
        
        L_s += transmittance_viewer_to_sample * in_scattered_light * transmittance_sun_to_sample * step_size;
    }
    return L_s;
}

float SunDisk(vec3 view_dir_world_norm, vec3 sun_dir_world_norm, float sun_angular_radius_rad) 
{
    float cos_alpha = dot(view_dir_world_norm, sun_dir_world_norm);
    float threshold_cos = cos(sun_angular_radius_rad);
    return smoothstep(threshold_cos - 0.0001, threshold_cos + 0.0001, cos_alpha);
}

void main() 
{    
    vec3 final_sky_color = vec3(0.0);
    vec3 view_dir_norm = normalize(fs_in.viewDir_world); 
    vec3 sun_dir_norm = normalize(params.sunDir); 

    final_sky_color = IntegrateSingleScattering(cameraPos_world_km, view_dir_norm, sun_dir_norm);

    float cam_altitude_km = length(cameraPos_world_km) - params.bottomRadiusKM;
    vec3 cam_up_dir = normalize(cameraPos_world_km);
    float sun_zenith_mu_at_cam = dot(cam_up_dir, sun_dir_norm);
    
    vec3 sun_transmittance_to_cam = SampleTransmittanceLUT(cam_altitude_km, sun_zenith_mu_at_cam);
    
    float sun_disk_intensity = SunDisk(view_dir_norm, sun_dir_norm, params.sunAngularRadius);
    final_sky_color += sun_disk_intensity * sun_transmittance_to_cam * params.solarIrradiance;

    final_sky_color *= params.exposure;
    // If I dont end up tonemapping/gamma correction here then I can just replace this shader with physicallyBasedSky_frag.glsl completely

    FragColor = vec4(final_sky_color, 1.0);
}