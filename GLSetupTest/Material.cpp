#include "Material.h"

namespace JLEngine
{
    Material::Material(uint32 handle, const std::string& name) : Resource(handle, name),
        baseColorFactor(1.0f, 1.0f, 1.0f, 1.0f), // Default white color
        metallicFactor(1.0f),                   // Fully metallic
        roughnessFactor(1.0f),                  // Fully rough
        emissiveFactor(0.0f, 0.0f, 0.0f),       // No emissive
        alphaMode("OPAQUE"),                    // Default opaque
        alphaCutoff(0.5f),                      // Default cutoff for mask
        doubleSided(false),                     // Default single-sided
        usesSpecularGlossinessWorkflow(false),  // Default metallic-roughness workflow
        diffuseFactor(1.0f, 1.0f, 1.0f, 1.0f),  // Default diffuse color
        specularFactor(1.0f, 1.0f, 1.0f)        // Default specular color
    {
        
    }
}