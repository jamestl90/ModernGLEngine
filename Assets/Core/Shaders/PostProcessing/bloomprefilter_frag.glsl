#version 460 core

in vec2 v_TexCoords;

out vec4 FragColor;

layout(binding = 0) uniform sampler2D u_Texture;

uniform float u_Threshold; 
uniform float u_Knee;      
                           
float Luminance(vec3 color) 
{
    return dot(color, vec3(0.2126, 0.7152, 0.0722));
}

void main() 
{
    vec3 hdrColor = texture(u_Texture, v_TexCoords).rgb;

    //float bright = Luminance(hdrColor);
    float bright = max(hdrColor.r, max(hdrColor.g, hdrColor.b)); 

    float softThresholdStart = u_Threshold - u_Knee; 
    float contribution = 0.0;

    if (bright > softThresholdStart) 
    { 
        if (u_Knee > 0.00001f) 
        { 
            float x = (bright - softThresholdStart) / (2.0f * u_Knee); 

            float ramp = (bright - (u_Threshold - u_Knee)) / max(u_Knee, 0.00001f); 
            contribution = clamp(ramp, 0.0, 1.0); 
        }
        else 
        { 
            contribution = step(u_Threshold, bright);
        }
    }
    
    //hdrColor.x = isnan(hdrColor.x) ? 0.0 : hdrColor.x;
    //hdrColor.y = isnan(hdrColor.y) ? 0.0 : hdrColor.y;
    //hdrColor.z = isnan(hdrColor.z) ? 0.0 : hdrColor.z;

    FragColor = vec4(hdrColor * contribution, 1.0);
}