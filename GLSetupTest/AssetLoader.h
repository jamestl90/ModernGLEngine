
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
	enum class NormalGen
	{
		Smooth,
		Flat
	};

	struct AssetGenerationSettings
	{
		bool GenerateNormals = true; // if missing generate normals
		bool GenerateTangents = true; // if missing generate tangents
		NormalGen NormalGenType = NormalGen::Smooth; // type of normals to generate
	};

	class AssetLoader
	{
	public:
		AssetLoader(ShaderManager* shaderManager, MeshManager* meshManager, 
			MaterialManager* materialManager, TextureManager* textureManager);

		std::vector<std::shared_ptr<Node>> LoadGLB(const std::string& glbFile);

		void SetGlobalGenerationSettings(AssetGenerationSettings& settings) { m_settings = settings; }

	protected:
		std::vector<Mesh*> loadMeshes(tinygltf::Model& model);
		std::vector<Material*> loadMaterials(tinygltf::Model& model);

		AssetGenerationSettings m_settings;

		ShaderManager* m_shaderManager;
		MeshManager* m_meshManager;
		MaterialManager* m_materialManager;
		TextureManager* m_textureManager;
	};
}

#endif