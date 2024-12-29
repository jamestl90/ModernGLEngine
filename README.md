Model Loading

Using tiny_gltf for loading scenes
Engine supports 3 vertex formats { POS, NORMAL, UV, TANGENT } and { POS, NORMAL } and { POS, UV }
For the former, tangents (x,y,z,w) are automatically generated if not present
ex. Mesh has POS, NORMAL and UV then tangents will be generated from that data
Multiple UV attributes will be supported later. 

If the glb/gltf file shows multiple primitives per mesh, they will be batched based on attribute setup

If all attributes are present we use a PBR shader. This will later support other shading models. 
If only POS and NORMAL are present, an un-textured shader is used to render a solid color
If only POS and UV are present, an un-lit shader is used to render a single textured mesh


Rendering Setup

Supports a basic deferred pipeline with ALBEDO(AO), NORMALS, METALLIC/ROUGHNESS and EMISSIVE multi textured render target
Meshes without UVs or without Normals are rendered separately 

![Screenshot](https://github.com/jamestl90/GLSetupTest/blob/main/Screenshots/pbr_gbuffer_debug.png)
<img src='https://github.com/jamestl90/GLSetupTest/blob/main/Screenshots/pbr_gbuffer_debug.png'/>
