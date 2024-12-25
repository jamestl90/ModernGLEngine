#ifndef MATERIAL_H
#define MATERIAL_H

#include <string>
#include <glm/glm.hpp>
#include <memory>

#include "Types.h"
#include "Resource.h"
#include "JLHelpers.h"

namespace JLEngine
{
    class Texture; 

    enum class AlphaMode
    {
        JL_OPAQUE,
        MASK,
        BLEND
    };

    class Material : public Resource
    {
    public:
        // Default constructor
        Material(const std::string& name);

        // Properties for PBR Metallic-Roughness workflow
        glm::vec4 baseColorFactor;            // Base color (RGBA)
        std::shared_ptr<Texture> baseColorTexture; // Texture for base color

        float metallicFactor;                // Metalness (0 = dielectric, 1 = metallic)
        float roughnessFactor;               // Surface roughness

        std::shared_ptr<Texture> metallicRoughnessTexture; // Combined metallic-roughness texture

        // Additional textures
        std::shared_ptr<Texture> normalTexture;           // Normal map
        std::shared_ptr<Texture> occlusionTexture;        // Ambient occlusion map
        std::shared_ptr<Texture> emissiveTexture;         // Emissive map

        glm::vec3 emissiveFactor;            // Emissive color

        // Alpha properties
        AlphaMode alphaMode;               // Alpha mode: "OPAQUE", "MASK", "BLEND"
        float alphaCutoff;                   // Alpha cutoff for "MASK" mode
        bool doubleSided;                    // Double-sided rendering flag

        bool castShadows = true;        // does not get rendered in depth pre-pass 
        bool receiveShadows = true;     // does not receive shadows in lighting pass

    private:
        // Helper methods to handle material properties can be added here
    };

    AlphaMode AlphaModeFromString(const std::string& alphaMode);
}

#endif