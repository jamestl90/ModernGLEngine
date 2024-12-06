#ifndef GLB_LOADER_H
#define GLB_LOADER_H

#include <glm/glm.hpp>
#include <tiny_gltf.h>
#include <string>
#include <iostream>
#include <functional>

#include "VertexBuffers.h"
#include "Batch.h"

namespace JLEngine
{
	class Node;
	class Material;
	class Texture;
	class TextureManager;
	class MaterialManager;
	class Mesh;
	class Graphics;
	class AssetLoader;

	struct MaterialAttributeKey
	{
		int materialIndex;     // Material index for the primitive
		std::string attributesKey; // Unique key representing the attribute layout

		MaterialAttributeKey(int matIdx, const std::string& attrKey)
			: materialIndex(matIdx), attributesKey(attrKey) {}

		bool operator==(const MaterialAttributeKey& other) const
		{
			return materialIndex == other.materialIndex &&
				attributesKey == other.attributesKey;
		}
	};

	class GLBLoader
	{
	public:
		GLBLoader(Graphics* graphics, AssetLoader* assetLoader);

		std::shared_ptr<Node> LoadGLB(const std::string& fileName);

	protected:
		std::shared_ptr<Node> ParseNode(const tinygltf::Model& model, const tinygltf::Node& gltfNode);
		Mesh* ParseMesh(const tinygltf::Model& model, int meshIndex);
		Material* ParseMaterial(const tinygltf::Model& model, const tinygltf::Material& gltfMaterial, int matIdx);
		Texture* ParseTexture(const tinygltf::Model& model, std::string& name, int textureIndex);
		void ParseTransform(std::shared_ptr<Node> node, const tinygltf::Node& gltfNode);

		std::shared_ptr<VertexBuffer> MergeVertexData(const tinygltf::Model& model, const std::vector<const tinygltf::Primitive*>& primitives);
		std::shared_ptr<IndexBuffer> MergeIndexData(const tinygltf::Model& model, const std::vector<const tinygltf::Primitive*>& primitives);
		std::shared_ptr<Batch> CreateBatch(const tinygltf::Model& model, const std::vector<const tinygltf::Primitive*>& primitives, MaterialAttributeKey key);

		bool LoadIndices(const tinygltf::Model& model, const tinygltf::Primitive& primitive, std::vector<unsigned int>& indices);
		bool LoadTangentAttribute(const tinygltf::Model& model, const tinygltf::Primitive& primitive, std::vector<float>& tangentData);
		bool LoadTexCoordAttribute(const tinygltf::Model& model, const tinygltf::Primitive& primitive, std::vector<float>& vertexData);
		bool LoadNormalAttribute(const tinygltf::Model& model, const tinygltf::Primitive& primitive, std::vector<float>& vertexData);
		void LoadPositionAttribute(const tinygltf::Model& model, const tinygltf::Primitive& primitive, std::vector<float>& vertexData);
		std::string GetAttributesKey(const std::map<std::string, int>& attributes);

		glm::vec4 GetVec4FromValue(const tinygltf::Value& value, const glm::vec4& defaultValue);
		glm::vec3 GetVec3FromValue(const tinygltf::Value& value, const glm::vec3& defaultValue);

		std::unordered_map<int, Mesh*> meshCache;
		std::unordered_map<int, Material*> materialCache;
		std::unordered_map<int, Texture*> textureCache;

		Graphics* m_graphics;
		AssetLoader* m_assetLoader;
	};
}

namespace std
{
	template <>
	struct hash<JLEngine::MaterialAttributeKey>
	{
		size_t operator()(const JLEngine::MaterialAttributeKey& key) const
		{
			return std::hash<int>()(key.materialIndex) ^ std::hash<std::string>()(key.attributesKey);
		}
	};
}


#endif