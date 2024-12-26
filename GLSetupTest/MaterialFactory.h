#ifndef MATERIAL_FACTORY_H
#define MATERIAL_FACTORY_H

#include "Material.h"
#include "ResourceManager.h"

namespace JLEngine
{
	class MaterialFactory
	{
	public:
        MaterialFactory(ResourceManager<Material>* materialManager)
        : m_materialManager(materialManager) {}

        std::shared_ptr<Material> CreateMaterial(const std::string& name)
        {
            return m_materialManager->Load(name, [&]()
            {
                auto mat = std::make_unique<Material>(name);

                return mat;
            });
        }

	protected:
        ResourceManager<Material>* m_materialManager;
	};
}

#endif