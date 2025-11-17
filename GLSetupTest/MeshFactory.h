#ifndef MESH_FACTORY_H
#define MESH_FACTORY_H

#include "ResourceManager.h"
#include "Mesh.h"

#include <memory>
#include <string>

namespace JLEngine
{
	class MeshFactory
	{
	public:
		MeshFactory(ResourceManager<Mesh>* meshManager)
			: m_meshManager(meshManager) {}

		std::shared_ptr<Mesh> CreateMesh(const std::string& name)
		{
			return m_meshManager->Load(name, [&]()
				{
					auto mesh = std::make_shared<Mesh>(name);
					return mesh;
				});
		}

	protected:
		ResourceManager<Mesh>* m_meshManager;
	};
}

#endif