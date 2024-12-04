
#ifndef ASSET_LOADER_H
#define ASSET_LOADER_H

#include <tiny_gltf.h>

#include "ShaderManager.h"
#include "TextureManager.h"
#include "MaterialManager.h"
#include "MeshManager.h"
#include "Node.h"
#include "Material.h"

namespace JLEngine
{
	class AssetLoader
	{
	public:
		AssetLoader(ShaderManager* shaderManager, MeshManager* meshManager, 
			MaterialManager* materialManager, TextureManager* textureManager);

		std::unique_ptr<Node> LoadGLB(const std::string& glbFile);

	protected:
		std::vector<Mesh*> loadMeshes(tinygltf::Model& model);
		std::vector<Material*> loadMaterials(tinygltf::Model& model);

		ShaderManager* m_shaderManager;
		MeshManager* m_meshManager;
		MaterialManager* m_materialManager;
		TextureManager* m_textureManager;
	};
}

#endif