#version 460 

in VS_OUT 
{
    vec3 viewDir_world;
} fs_in;

layout(location = 0) out vec4 FragColor;

#include "common.glsl" 

uniform vec3 cameraPos_world_km; // Camera position (km, planet center is origin)
layout(binding = 0) uniform sampler2D transmittanceLUT_sampler;

const int NUM_VIEW_RAY_SAMPLES = 16; // Reduced for dynamic version, adjust for quality/perf
const int NUM_VIEW_PATH_OPTICAL_DEPTH_SAMPLES = 8;

vec3 SampleTransmittanceLUT(float altitude_km, float view_zenith_mu) 
{
    float atmosphere_thickness_km = params.topRadiusKM - params.bottomRadiusKM;
    // Clamping altitude happens inside AltitudeCosAngleToUV
    vec2 uv = AltitudeCosAngleToUV(altitude_km, view_zenith_mu, atmosphere_thickness_km);
    return texture(transmittanceLUT_sampler, uv).rgb;
}

vec3 IntegrateSingleScattering(vec3 ray_origin_world, vec3 ray_dir_world_norm, vec3 sun_dir_world_norm) 
{
    vec3 L_s = vec3(0.0); 

    // Intersections of view ray with atmosphere and planet
    vec2 t_atmosphere_intersect = IntersectSphere(ray_origin_world, ray_dir_world_norm, params.topRadiusKM);
    vec2 t_planet_intersect = IntersectSphere(ray_origin_world, ray_dir_world_norm, params.bottomRadiusKM);

    float ray_start_offset = 0.0; 
    float ray_end_dist;

    if (length(ray_origin_world) > params.topRadiusKM) { // Camera above atmosphere
        if (t_atmosphere_intersect.x > 0.0 && t_atmosphere_intersect.y > t_atmosphere_intersect.x) {
            ray_start_offset = t_atmosphere_intersect.x;
            ray_end_dist = t_atmosphere_intersect.y;
        } else { 
            return vec3(0.0); // No scattering
        }
    } else { // Camera inside or on boundary
        ray_start_offset = 0.0; // Integration starts from camera
        ray_end_dist = t_atmosphere_intersect.y; // To edge of atmosphere
    }

    // Check ground collision
    if (t_planet_intersect.x > ray_start_offset && t_planet_intersect.x < ray_end_dist) {
         ray_end_dist = t_planet_intersect.x;
    }
    
    float segment_length = ray_end_dist - ray_start_offset;
    if (segment_length <= 1e-3f) return vec3(0.0);

    float step_size = segment_length / float(NUM_VIEW_RAY_SAMPLES);

    for (int i = 0; i < NUM_VIEW_RAY_SAMPLES; ++i) {
        float t_sample = ray_start_offset + (float(i) + 0.5) * step_size;
        vec3 p_sample_world = ray_origin_world + ray_dir_world_norm * t_sample; 
        float p_sample_altitude_km = length(p_sample_world) - params.bottomRadiusKM;

        if (p_sample_altitude_km < 0.0 || p_sample_altitude_km > (params.topRadiusKM - params.bottomRadiusKM)) {
            continue; 
        }

        // Transmittance from camera to sample (still calculated dynamically for accuracy)
        vec3 transmittance_viewer_to_sample = exp(-OpticalDepth(ray_origin_world, p_sample_world, NUM_VIEW_PATH_OPTICAL_DEPTH_SAMPLES));

        // Transmittance from sun to sample point (USE LUT)
        vec3 p_sample_up_dir = normalize(p_sample_world);
        float sun_zenith_mu_at_sample = dot(p_sample_up_dir, sun_dir_world_norm);
        vec3 transmittance_sun_to_sample = SampleTransmittanceLUT(p_sample_altitude_km, sun_zenith_mu_at_sample); // <-- LUT USED HERE
        
        // Scattering phase functions
        float cos_scatter_angle = dot(ray_dir_world_norm, sun_dir_world_norm);
        vec3 rayleigh_contrib = GetScatteringCoeffRayleigh(p_sample_altitude_km) * RayleighPhase(cos_scatter_angle);
        vec3 mie_contrib = GetScatteringCoeffMie(p_sample_altitude_km) * HenyeyGreensteinPhase(cos_scatter_angle, params.miePhaseG);
        
        vec3 total_scattering_at_sample = rayleigh_contrib + mie_contrib;
        vec3 in_scattered_light = total_scattering_at_sample * params.solarIrradiance;
        
        L_s += transmittance_viewer_to_sample * in_scattered_light * transmittance_sun_to_sample * step_size;
    }
    return L_s;
}

// Renders sun disk
float SunDisk(vec3 view_dir_world_norm, vec3 sun_dir_world_norm, float sun_angular_radius_rad) {
    float cos_alpha = dot(view_dir_world_norm, sun_dir_world_norm);
    float threshold_cos = cos(sun_angular_radius_rad);
    return smoothstep(threshold_cos - 0.0001, threshold_cos + 0.0001, cos_alpha);
}

void main() 
{    
    vec3 final_sky_color = vec3(0.0);
    // Normalize the incoming interpolated view direction
    vec3 view_dir_norm = normalize(fs_in.viewDir_world); 
    vec3 sun_dir_norm = normalize(params.sunDir); 

    // Calculate single-scattered sky luminance using view_dir_norm
    final_sky_color = IntegrateSingleScattering(cameraPos_world_km, view_dir_norm, sun_dir_norm);

    // Add sun disk contribution
    float cam_altitude_km = length(cameraPos_world_km) - params.bottomRadiusKM;
    vec3 cam_up_dir = normalize(cameraPos_world_km);
    float sun_zenith_mu_at_cam = dot(cam_up_dir, sun_dir_norm);
    // Transmittance for direct sun light (USE LUT)
    vec3 sun_transmittance_to_cam = SampleTransmittanceLUT(cam_altitude_km, sun_zenith_mu_at_cam); // <-- LUT USED HERE
    
    float sun_disk_intensity = SunDisk(view_dir_norm, sun_dir_norm, params.sunAngularRadius);
    final_sky_color += sun_disk_intensity * sun_transmittance_to_cam * params.solarIrradiance;

    // Exposure and basic tonemapping (if outputting LDR)
    final_sky_color *= params.exposure;
    //final_sky_color = final_sky_color / (final_sky_color + vec3(1.0)); // Reinhard
    //final_sky_color = pow(final_sky_color, vec3(1.0/2.2)); // Gamma correction

    FragColor = vec4(final_sky_color, 1.0);
}