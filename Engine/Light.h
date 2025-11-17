#ifndef LIGHT_H
#define LIGHT_H

#include <glm/glm.hpp>

namespace JLEngine
{
    enum class LightType : int32_t { Point = 0, Directional, Spot };

    struct LightGPU
    {
        glm::vec3 position;     
        float intensity;        

        glm::vec3 color;        
        float radius;           

        glm::vec3 direction;    
        float spotAngleOuter;   

        int32_t type;           
        float spotAngleInner;   
        int32_t enabled;        
        int32_t castsShadows;   
    };
}

#endif