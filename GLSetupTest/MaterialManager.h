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
		MaterialManager(Graphics* graphics) : m_graphics(graphics) {}

		Material* CreateMaterial(const std::string& name) 
		{
			return Add(name, [&]()
				{
					auto mat = std::make_unique<Material>(GenerateHandle(), name);

					return mat;
				});
		}

	private:
		Graphics* m_graphics;
	};
}

#endif