#ifndef MATERIAL_MANAGER_H
#define MATERIAL_MANAGER_H

#include "ResourceManager.h"
#include "Graphics.h"
#include "Material.h"

namespace JLEngine
{
	class Material;

	class MaterialManager : ResourceManager<Material>
	{
	public:
		MaterialManager(Graphics* graphics);

		Material* CreateMaterial(const std::string& name) 
		{
			return Add(name, [&]()
				{
					auto mat = std::make_unique<Material>(GenerateHandle(), name);

					return mat;
				});
		}

		Material* GetDefaultMaterial() { return m_defaultMat; }

	private:
		Graphics* m_graphics;

		Material* m_defaultMat;
	};
}

#endif