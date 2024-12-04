#ifndef LIGHT_H
#define LIGHT_H

#include <glm/glm.hpp>

namespace JLEngine
{
    enum class LightType { Point, Directional, Spot };

    struct Light 
    {
        glm::vec3 position;     // Light position
        float intensity;        // Intensity
        glm::vec3 color;        // Light color
        float radius;           // Attenuation radius
        glm::vec3 direction;    // Spotlight direction
        float angle;            // Spotlight cone angle
        LightType type;         // Light type
        bool isStatic;          // Static or dynamic
    };
}

#endif