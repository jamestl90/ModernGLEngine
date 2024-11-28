#ifndef MATERIAL_H
#define MATERIAL_H

#include <string>
#include <glm/glm.hpp>
#include <memory>

namespace JLEngine
{
    class Texture; 

    class Material
    {
    public:
        // Default constructor
        Material();

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
        std::string alphaMode;               // Alpha mode: "OPAQUE", "MASK", "BLEND"
        float alphaCutoff;                   // Alpha cutoff for "MASK" mode
        bool doubleSided;                    // Double-sided rendering flag

        // Extensions (e.g., KHR_materials_pbrSpecularGlossiness)
        bool usesSpecularGlossinessWorkflow; // Flag for using specular-glossiness workflow
        glm::vec4 diffuseFactor;             // Diffuse color (for specular-glossiness)
        std::shared_ptr<Texture> diffuseTexture;         // Diffuse texture
        glm::vec3 specularFactor;            // Specular color (for specular-glossiness)
        std::shared_ptr<Texture> specularGlossinessTexture; // Combined specular-glossiness texture

    private:
        // Helper methods to handle material properties can be added here
    };
}

#endif