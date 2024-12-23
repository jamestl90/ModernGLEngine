#include "Material.h"
#include <stdexcept>

namespace JLEngine
{
    Material::Material(const std::string& name) : Resource(name),
        baseColorFactor(1.0f, 1.0f, 1.0f, 1.0f),
        metallicFactor(1.0f),                   
        roughnessFactor(1.0f),                  
        emissiveFactor(0.0f, 0.0f, 0.0f),       
        alphaMode(AlphaMode::JL_OPAQUE),                    
        alphaCutoff(0.5f),                      
        doubleSided(false),   
        receiveShadows(true),
        castShadows(true)
    {
        
    }

    AlphaMode AlphaModeFromString(const std::string& alphaMode)
    {
        auto lowerStr = JLEngine::Str::ToLower(alphaMode);
        if (JLEngine::Str::Contains(lowerStr, "mask"))
            return AlphaMode::MASK;
        if (JLEngine::Str::Contains(lowerStr, "opaque"))
            return AlphaMode::JL_OPAQUE;
        if (JLEngine::Str::Contains(lowerStr, "blend"))
            return AlphaMode::BLEND;

        throw std::invalid_argument("alphaMode is incorrect");
    }
}