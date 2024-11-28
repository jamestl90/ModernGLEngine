#include "AssetLoader.h"
#include "GltfLoader.h"
#include "Geometry.h"

namespace JLEngine
{
    AssetLoader::AssetLoader(ShaderManager* shaderManager, MeshManager* meshManager, MaterialManager* materialManager, TextureManager* textureManager)
        : m_shaderManager(shaderManager), m_meshManager(meshManager), m_materialManager(materialManager), m_textureManager(textureManager)
    {

    }
    Node* AssetLoader::LoadGLB(const std::string& glbFile)
    {
		tinygltf::TinyGLTF loader;
		tinygltf::Model model;
		std::string err;
		std::string warn;

		bool ret = loader.LoadBinaryFromFile(&model, &err, &warn, glbFile);
		if (!ret)
		{
			std::cout << err << std::endl;
			return nullptr;
		}

		auto meshes = loadMeshes(model);
    }

	std::vector<Mesh*> AssetLoader::loadMeshes(tinygltf::Model& model)
	{
		auto meshes = std::vector<Mesh*>();

		for (const auto& mesh : model.meshes)
		{
			if (mesh.primitives.size() == 1)
			{
				auto jlmesh = PrimitiveFromMesh(model, mesh.primitives[0], m_meshManager);
				meshes.push_back(jlmesh);
			}
			else if (mesh.primitives.size() > 1)
			{
				auto jlmesh = MergePrimitivesToMesh(model, mesh, m_meshManager);
				meshes.push_back(jlmesh);
			}
		}

		return meshes;
	}
}