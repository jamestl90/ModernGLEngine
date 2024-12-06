#include "Material.h"

namespace JLEngine
{
    Material::Material(const std::string& name) : Resource(name),
        baseColorFactor(1.0f, 1.0f, 1.0f, 1.0f),
        metallicFactor(1.0f),                   
        roughnessFactor(1.0f),                  
        emissiveFactor(0.0f, 0.0f, 0.0f),       
        alphaMode("OPAQUE"),                    
        alphaCutoff(0.5f),                      
        doubleSided(false),                     
        usesSpecularGlossinessWorkflow(false),  
        diffuseFactor(1.0f, 1.0f, 1.0f, 1.0f),  
        specularFactor(1.0f, 1.0f, 1.0f)        
    {
        
    }
}