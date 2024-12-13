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
        Texture* baseColorTexture; // Texture for base color

        float metallicFactor;                // Metalness (0 = dielectric, 1 = metallic)
        float roughnessFactor;               // Surface roughness

        Texture* metallicRoughnessTexture; // Combined metallic-roughness texture

        // Additional textures
        Texture* normalTexture;           // Normal map
        Texture* occlusionTexture;        // Ambient occlusion map
        Texture* emissiveTexture;         // Emissive map

        glm::vec3 emissiveFactor;            // Emissive color

        // Alpha properties
        AlphaMode alphaMode;               // Alpha mode: "OPAQUE", "MASK", "BLEND"
        float alphaCutoff;                   // Alpha cutoff for "MASK" mode
        bool doubleSided;                    // Double-sided rendering flag

        // Extensions (e.g., KHR_materials_pbrSpecularGlossiness)
        bool usesSpecularGlossinessWorkflow; // Flag for using specular-glossiness workflow
        glm::vec4 diffuseFactor;             // Diffuse color (for specular-glossiness)
        Texture* diffuseTexture;         // Diffuse texture
        glm::vec3 specularFactor;            // Specular color (for specular-glossiness)
        Texture* specularGlossinessTexture; // Combined specular-glossiness texture

    private:
        // Helper methods to handle material properties can be added here
    };

    AlphaMode AlphaModeFromString(const std::string& alphaMode);
}

#endif