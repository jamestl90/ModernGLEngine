Engine is based on Modern OpenGL 4.6 and utilises it's features such as DSA (Direct State Access), bindless textures (GL_ARB_bindless_texture), SSBOs (Shader Storage Buffer Objects) and Multi-Draw Indirect. 

https://trello.com/b/9M6YkL0b/opengl-engine-jlengine

Rendering Setup:

Supports a basic deferred PBR pipeline with ALBEDO(AO), NORMALS, METALLIC/ROUGHNESS and EMISSIVE being stored in the gbuffer. 
All geometry with the same vertex layout are batched in a single vertex array object and can be drawn with a single call to glMultiDrawElementsIndirect. 

The lighting is still a work in progress. Currently there is a single directional light and some ambient and specular indirect lighting from the HDRi Sky textures. These maps are generated at program launch - Cubemap, Irradiance Map, Prefiltered Environment Map and BRDF (bi-directional reflectance distribution function). 

Real-time debugging of the GBuffer, directional shadow-map and sky textures is implemented. 

Libraries Used:
GLFW - for window creation and management 
glad - for opengl bindings
tiny_gltf - for loading GLB and GLTF files
stb_image - for loading images
Dear IMGUI - for debug UI rendering

<img src='https://github.com/jamestl90/GLSetupTest/blob/main/Screenshots/pbr_hdri_reflections.png'/>
<img src='https://github.com/jamestl90/GLSetupTest/blob/main/Screenshots/pbr_hdri_reflections2.png'/>
<img src='https://github.com/jamestl90/GLSetupTest/blob/main/Screenshots/pbr_hdri_reflections3.png'/>
<img src='https://github.com/jamestl90/GLSetupTest/blob/main/Screenshots/pbr_hdri_reflections4.png'/>
<img src='https://github.com/jamestl90/GLSetupTest/blob/main/Screenshots/pbr_hdri_sky_debug.png'/>
<img src='https://github.com/jamestl90/GLSetupTest/blob/main/Screenshots/pbr_gbuffer_debug.png'/>
<img src='https://github.com/jamestl90/GLSetupTest/blob/main/Screenshots/instancedSkinnedMeshes.png'/>
