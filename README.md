Engine is based on Modern OpenGL 4.6 and utilises it's features such as DSA (Direct State Access), bindless textures (GL_ARB_bindless_texture), SSBOs (Shader Storage Buffer Objects) and Multi-Draw Indirect. 

https://trello.com/b/9M6YkL0b/opengl-engine-jlengine


Recently Completed: Bloom post processing effect (Screenshots not uploaded yet) </br> 
Current feature in progress: Dynamic Diffuse Global Illumination 

Generate a grid of irradiance probes to populate the space in the world, using AABB collision to determine valid probes. Red probes = skip processing
<img src='https://github.com/jamestl90/GLSetupTest/blob/main/Screenshots/IrradianceProbeGrid.png'/>

Cycle through each probe and visualize the rays it casts. White ray = hit, black ray = miss/sky 
<img src='https://github.com/jamestl90/GLSetupTest/blob/main/Screenshots/Probe_ray_debug.png'/>

AABB visualization of the world. For indoor lighting will need to refine this. 
<img src='https://github.com/jamestl90/GLSetupTest/blob/main/Screenshots/AABB_Debug_draw.png'/>


Rendering Setup:

Supports a basic deferred PBR pipeline with ALBEDO(AO), NORMALS, METALLIC/ROUGHNESS and EMISSIVE being stored in the gbuffer. 
All geometry with the same vertex layout are batched in a single vertex array object and can be drawn with a single call to glMultiDrawElementsIndirect. 

The old HDRi sky cubemap texture has been replaced with a Physically based sky model based on these two documents:  
https://cpp-rendering.io/sky-and-atmosphere-rendering/  
https://sebh.github.io/publications/egsr2020.pdf  
With this I've added a Sky Probe which "snapshots" the pb sky into a low res cubemap/prefiltered map. The lighting shader now samples the prefiltered map for sky reflections. 

~~The lighting is still a work in progress. Currently there is a single directional light and some ambient and specular indirect lighting from the HDRi Sky textures. These maps are generated at program launch - Cubemap, Irradiance Map, Prefiltered Environment Map and BRDF (bi-directional reflectance distribution function).~~ 

Libraries Used:
GLFW - for window creation and management 
glad - for opengl bindings
tiny_gltf - for loading GLB and GLTF files
stb_image - for loading images
Dear IMGUI - for debug UI rendering

**NEW SKY METHOD** 
Physically based sky and reflection sky probe:
<img src='https://github.com/jamestl90/GLSetupTest/blob/main/Screenshots/pb_sky_1.png'/>
<img src='https://github.com/jamestl90/GLSetupTest/blob/main/Screenshots/pb_sky_2.png'/>
<img src='https://github.com/jamestl90/GLSetupTest/blob/main/Screenshots/pb_sky_3.png'/>

Real-time debugging of the GBuffer, directional shadow-map and sky textures is implemented. 

GBuffer debug view:
<img src='https://github.com/jamestl90/GLSetupTest/blob/main/Screenshots/pbr_gbuffer_debug.png'/>

Instanced Animated/Skinned mesh rendering:
<img src='https://github.com/jamestl90/GLSetupTest/blob/main/Screenshots/instancedSkinnedMeshes.png'/>

**OLD METHOD** 
HDRI Sky reflections and PBR workflow
<img src='https://github.com/jamestl90/GLSetupTest/blob/main/Screenshots/pbr_hdri_reflections.png'/>
<img src='https://github.com/jamestl90/GLSetupTest/blob/main/Screenshots/pbr_hdri_reflections2.png'/>
<img src='https://github.com/jamestl90/GLSetupTest/blob/main/Screenshots/pbr_hdri_reflections3.png'/>
<img src='https://github.com/jamestl90/GLSetupTest/blob/main/Screenshots/pbr_hdri_reflections4.png'/>
<img src='https://github.com/jamestl90/GLSetupTest/blob/main/Screenshots/pbr_hdri_sky_debug.png'/>
