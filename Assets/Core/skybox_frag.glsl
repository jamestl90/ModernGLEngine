#version 460 core

in vec3 v_FragPos;       
in vec3 v_Normal;        
in vec2 v_TexCoords;     

out vec4 FragColor;         

uniform sampler2D u_SkyboxTexture;    

void main() 
{
    vec4 textureColor = texture(u_SkyboxTexture, v_TexCoords);

    FragColor = vec4(textureColor.rgb, 1.0);
}
