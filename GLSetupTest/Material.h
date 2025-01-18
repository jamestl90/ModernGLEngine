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

    struct alignas(16) MaterialGPU
    {
        glm::vec4 baseColorFactor;            // 16 bytes
        glm::vec4 emissiveFactor;             // 16 bytes
        uint64_t baseColorHandle;             // 8 bytes
        uint64_t metallicRoughnessHandle;     // 8 bytes
        uint64_t normalHandle;                // 8 bytes
        uint64_t occlusionHandle;             // 8 bytes
        uint64_t emissiveHandle;              // 8 bytes
        float metallicFactor;                 // 4 bytes
        float roughnessFactor;                // 4 bytes
        float alphaCutoff;                    // 4 bytes
        uint32_t alphaMode;                   // 4 bytes
        uint32_t receiveShadows;              // 4 bytes      
        float transmissionFactor;             // 4 bytes: Strength of transmission (0.0 - 1.0)
        float refractionIndex;                // 4 bytes: Index of refraction (e.g., 1.5 for glass)
        float thickness;                      // 4 bytes: Thickness for volumetric effects (optional)
        float padding1;                       // 4 bytes: Padding to maintain 16-byte alignment
        glm::vec3 attenuationColor;           // 12 bytes: RGB color for attenuation
        float attenuationDistance;            // 4 bytes: Distance for light attenuation 
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

        bool useTransparency = false;
        float transmissionFactor;             
        float refractionIndex;                
        float thickness;
        glm::vec3 attenuationColor;           
        float attenuationDistance;            

        glm::vec2 offset = glm::vec2(0.0f);
        glm::vec2 scale = glm::vec2(1.0f);

    private:
        // Helper methods to handle material properties can be added here
    };

    AlphaMode AlphaModeFromString(const std::string& alphaMode);
}

#endif