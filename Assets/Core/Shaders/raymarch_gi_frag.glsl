#version 460

// Output
out vec3 MetallicReflection;

// Texture bindings
layout(binding = 0) uniform sampler2D albedoTex;
layout(binding = 1) uniform sampler2D normalTex;
layout(binding = 2) uniform sampler2D metallicRoughnessTex;
layout(binding = 3) uniform sampler2D depthTex;
layout(binding = 4) uniform sampler2D previousFrameIndirect;
layout(binding = 5) uniform sampler2D directLightTex;

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

// Uniforms
uniform float u_Near;
uniform float u_Far;

// Vertex inputs
in vec2 v_TexCoords;

// Constants
const int MAX_STEPS = 100;
const float STEP_SIZE = 0.1;
const float HIT_THRESHOLD = 0.01;

struct RayMarchResult
{
    vec3 color;
    vec3 scenePos;
    float depthDiff;
    vec2 hitUV;
    bool hit;
};

// Linearize depth
float LinearizeDepth(float depth, float near, float far) 
{
    float z = depth * 2.0 - 1.0;
    return (2.0 * near) / (far + near - z * (far - near));
}

// Convert UV to world space position
vec3 ReconstructWorldPosition(vec2 uv, float linearDepth, mat4 inverseView, mat4 inverseProj)
{
    vec4 ndc = vec4(uv * 2.0 - 1.0, 1.0, 1.0);
    vec4 viewSpacePos = inverseProj * ndc;
    viewSpacePos.xyz /= viewSpacePos.w;
    vec3 viewPos = viewSpacePos.xyz * linearDepth;
    vec4 worldPos = inverseView * vec4(viewPos, 1.0);
    return worldPos.xyz;
}

// Convert UV to View-Space Direction
vec3 ScreenToViewDir(vec2 uv, mat4 invProj)
{
    vec4 ndc = vec4(uv * 2.0 - 1.0, 1.0, 1.0);
    vec4 viewSpaceDir = invProj * ndc;
    return normalize(viewSpaceDir.xyz);
}

// Ray marching function with fixed UV sampling
RayMarchResult RayMarch(vec3 startPos, vec3 rayDir, mat4 invView, mat4 invProj)
{
    RayMarchResult result;
    result.color = vec3(0.02);  
    result.scenePos = vec3(0.0);
    result.depthDiff = 1.0;
    result.hitUV = vec2(0.0);
    result.hit = false;

    vec3 currentPos = startPos + rayDir * STEP_SIZE;
    vec2 bestUV = vec2(0.0);
    vec3 bestColor = vec3(0.02); 

    for (int i = 0; i < MAX_STEPS; i++)
    {
        vec4 projPos = projMatrix * viewMatrix * vec4(currentPos, 1.0);
        projPos.xyz /= projPos.w;
        vec2 screenUV = projPos.xy * 0.5 + 0.5;
        screenUV = clamp(screenUV, vec2(0.001), vec2(0.999));

        if (screenUV.x < 0.0 || screenUV.x > 1.0 || screenUV.y < 0.0 || screenUV.y > 1.0)
        {
            break;
        }

        float sceneDepth = texture(depthTex, screenUV).r;
        float sceneLinearDepth = LinearizeDepth(sceneDepth, u_Near, u_Far);
        vec3 scenePos = ReconstructWorldPosition(screenUV, sceneLinearDepth, invView, invProj);

        float depthDiff = abs(currentPos.z - scenePos.z);
        if (depthDiff < (HIT_THRESHOLD * sceneLinearDepth))
        {
            result.hit = true;
            bestUV = screenUV;
            bestColor = texture(albedoTex, bestUV).rgb;  
            break;
        }

        currentPos += rayDir * STEP_SIZE;
    }

    result.color = bestColor;
    result.hitUV = bestUV;
    return result;
}


void main()
{    
    mat4 inverseView = inverse(viewMatrix);
    mat4 inverseProj = inverse(projMatrix);

    vec3 fragNorm = normalize(texture(normalTex, v_TexCoords).xyz);
    float depth = texture(depthTex, v_TexCoords).r;
    float linearDepth = LinearizeDepth(depth, u_Near, u_Far);

    vec3 fragWorldPos = ReconstructWorldPosition(v_TexCoords, linearDepth, inverseView, inverseProj);
    vec3 fragCamPos = camPos.xyz;
    vec3 viewDir = normalize(fragCamPos - fragWorldPos);
    
    vec3 viewNormal = normalize((viewMatrix * vec4(fragNorm, 0.0)).xyz);
    vec3 viewRayDir = reflect(-viewDir, viewNormal);  
    vec3 rayDir = (inverseView * vec4(viewRayDir, 0.0)).xyz;  // Convert to world-space

    vec2 screenCenterOffset = abs(v_TexCoords - 0.5);
    float edgeFactor = clamp(1.0 - (screenCenterOffset.x + screenCenterOffset.y), 0.3, 1.0);
    rayDir *= edgeFactor;

    float metallic = texture(metallicRoughnessTex, v_TexCoords).b;
    
    if (metallic <= 0.001)
    {
        MetallicReflection = vec3(0.0);
        return;
    }

    RayMarchResult result = RayMarch(fragWorldPos, rayDir, inverseView, inverseProj);

    // debugging
    // MetallicReflection = vec3(result.hitUV, 0.0);
    // MetallicReflection = vec3(result.hit ? 1.0 : 0.0);  // debug hit detection
    // MetallicReflection = result.color; 

    MetallicReflection = result.color;  // final reflection color
}
