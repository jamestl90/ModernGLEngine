<h1>Description</h1>Engine is based on Modern OpenGL 4.6 and utilises it's features such as DSA (Direct State Access), bindless textures (GL_ARB_bindless_texture), SSBOs (Shader Storage Buffer Objects) and Multi-Draw Indirect. 

<h1>Libraries</h1>
GLFW - for window creation and management </br>
glad - for opengl bindings</br>
tiny_gltf - for loading GLB and GLTF files</br>
stb_image - for loading images</br>
Dear IMGUI - for debug UI rendering</br>


<h1>Features</h1> 

See https://trello.com/b/9M6YkL0b/opengl-engine-jlengine for task list </br>

<h2>Rendering</h2>

Supports a basic deferred PBR pipeline with ALBEDO(AO), NORMALS, METALLIC/ROUGHNESS and EMISSIVE being stored in the gbuffer. 
All geometry with the same vertex layout are batched in a single vertex array object and can be drawn with a single call to glMultiDrawElementsIndirect. I would like to extend this in the future to be more flexible which would help in dynamically adding/removing objects without messing with batched static geometry too much. 
<h2>Current feature in progress: Dynamic Diffuse Global Illumination </h2>
Generate a grid of irradiance probes to populate the space in the world, using AABB collision to determine valid probes. Red probes = skip processing
<img src='https://github.com/jamestl90/GLSetupTest/blob/main/Screenshots/IrradianceProbeGrid.png'/>

Cycle through each probe and visualize the rays it casts. White ray = hit, black ray = miss/sky 
<img src='https://github.com/jamestl90/GLSetupTest/blob/main/Screenshots/Probe_ray_debug.png'/>

AABB visualization of the world. For indoor lighting will need to refine this. 
<img src='https://github.com/jamestl90/GLSetupTest/blob/main/Screenshots/AABB_Debug_draw.png'/>

<h3>Bloom post processing effect</h3> Modern technique using prefiltering/upsampling/downsampling (Screenshots not uploaded yet) </br> 
<h3>Cascade Shadowmapping</h3> Implemented based on https://learnopengl.com/Guest-Articles/2021/CSM 

<h2>**NEW SKY METHOD** Physically based sky</h2>
The old HDRi sky cubemap texture has been replaced with a Physically based sky model based on these two documents:  
https://cpp-rendering.io/sky-and-atmosphere-rendering/  
https://sebh.github.io/publications/egsr2020.pdf  
With this I've added a Sky Probe which "snapshots" the pb sky into a low res cubemap/prefiltered map. The lighting shader now samples the prefiltered map for sky reflections. 
<img src='https://github.com/jamestl90/GLSetupTest/blob/main/Screenshots/pb_sky_1.png'/>
<img src='https://github.com/jamestl90/GLSetupTest/blob/main/Screenshots/pb_sky_2.png'/>
<img src='https://github.com/jamestl90/GLSetupTest/blob/main/Screenshots/pb_sky_3.png'/>


<h2>GBuffer debug view</h2>
Real-time debugging of the GBuffer
<img src='https://github.com/jamestl90/GLSetupTest/blob/main/Screenshots/pbr_gbuffer_debug.png'/>

Instanced Animated/Skinned mesh rendering:
<img src='https://github.com/jamestl90/GLSetupTest/blob/main/Screenshots/instancedSkinnedMeshes.png'/>

<h2>**OLD HDRI SKY METHOD**</h2>
HDRI Sky reflections and PBR workflow
<img src='https://github.com/jamestl90/GLSetupTest/blob/main/Screenshots/pbr_hdri_reflections.png'/>
<img src='https://github.com/jamestl90/GLSetupTest/blob/main/Screenshots/pbr_hdri_reflections2.png'/>
<img src='https://github.com/jamestl90/GLSetupTest/blob/main/Screenshots/pbr_hdri_reflections3.png'/>
<img src='https://github.com/jamestl90/GLSetupTest/blob/main/Screenshots/pbr_hdri_reflections4.png'/>
<img src='https://github.com/jamestl90/GLSetupTest/blob/main/Screenshots/pbr_hdri_sky_debug.png'/>
