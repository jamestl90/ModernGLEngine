#version 460 core

uniform samplerCube u_CubeMap;   // Cube map sampler
uniform vec3 u_Direction;        // The direction vector for the specific face

in vec2 v_TexCoords;
out vec4 FragColor;

void main()
{
    vec3 dir;
    if (u_Direction.x != 0.0) 
    { 
        dir = normalize(vec3(u_Direction.x, 2.0 * v_TexCoords.y - 1.0, 1.0 - 2.0 * v_TexCoords.x));
    }
    else if (u_Direction.y != 0.0) 
    { 
        dir = normalize(vec3(1.0 - 2.0 * v_TexCoords.x, u_Direction.y, 2.0 * v_TexCoords.y - 1.0));
    }
    else if (u_Direction.z != 0.0) 
    { 
        dir = normalize(vec3(2.0 * v_TexCoords.x - 1.0, 2.0 * v_TexCoords.y - 1.0, u_Direction.z));
    }

    vec3 col = texture(u_CubeMap, dir).rgb;
    // Sample the cube map at the fixed direction
    FragColor = vec4(col, 1.0);
}