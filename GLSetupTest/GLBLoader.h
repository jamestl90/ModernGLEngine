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

	struct MaterialVertexAttributeKey
	{
		int materialIndex;     // Material index for the primitive
		VertexAttribKey attributesKey; // Unique key representing the attribute layout

		MaterialVertexAttributeKey(int matIdx, VertexAttribKey attrKey)
			: materialIndex(matIdx), attributesKey(attrKey) {}

		bool operator==(const MaterialVertexAttributeKey& other) const
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
		std::shared_ptr<Batch> CreateBatch(const tinygltf::Model& model, const std::vector<const tinygltf::Primitive*>& primitives, MaterialVertexAttributeKey key);
		
		bool LoadIndices(const tinygltf::Model& model, const tinygltf::Primitive& primitive, std::vector<unsigned int>& indices);
		bool LoadTangentAttribute(const tinygltf::Model& model, const tinygltf::Primitive& primitive, std::vector<float>& tangentData);
		bool LoadTexCoordAttribute(const tinygltf::Model& model, const tinygltf::Primitive& primitive, std::vector<float>& vertexData);
		bool LoadTexCoord2Attribute(const tinygltf::Model& model, const tinygltf::Primitive& primitive, std::vector<float>& vertexData);
		bool LoadNormalAttribute(const tinygltf::Model& model, const tinygltf::Primitive& primitive, std::vector<float>& vertexData);
		void LoadPositionAttribute(const tinygltf::Model& model, const tinygltf::Primitive& primitive, std::vector<float>& vertexData);
		
		void SetupInstancing(Mesh* mesh, const std::vector<std::shared_ptr<Node>>& nodes) const;
		void GenerateMissingAttributes(std::vector<float>& positions, std::vector<float>& normals, const std::vector<float>& texCoords, std::vector<float>& tangents, const std::vector<uint32_t>& indices, VertexAttribKey key);
		void BatchLoadAttributes(const tinygltf::Model& model, const std::vector<const tinygltf::Primitive*>& primitives, std::vector<float>& positions, std::vector<float>& normals, std::vector<float>& texCoords, std::vector<float>& texCoords2, std::vector<float>& tangents, std::vector<uint32_t>& indices, uint32_t& indexOffset, VertexAttribKey key);
		void LoadAttributes(const tinygltf::Model& model, const tinygltf::Primitive* primitives, std::vector<float>& positions, std::vector<float>& normals, std::vector<float>& texCoords, std::vector<float>& tangents, std::vector<uint32_t>& indices, uint32_t& indexOffset);
		
		glm::vec4 GetVec4FromValue(const tinygltf::Value& value, const glm::vec4& defaultValue);
		glm::vec3 GetVec3FromValue(const tinygltf::Value& value, const glm::vec3& defaultValue);

	private:
		std::unordered_map<int, Mesh*> meshCache;
		std::unordered_map<int, Material*> materialCache;
		std::unordered_map<int, Texture*> textureCache;
		std::unordered_map<int, std::vector<std::shared_ptr<Node>>> meshNodeReferences;

		Graphics* m_graphics;
		AssetLoader* m_assetLoader;
	};
}

namespace std
{
	template <>
	struct hash<JLEngine::MaterialVertexAttributeKey>
	{
		size_t operator()(const JLEngine::MaterialVertexAttributeKey& key) const
		{
			size_t seed = 0;
			hash<int> hasher;
			seed ^= hasher(key.materialIndex) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			seed ^= hasher(key.attributesKey) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			return seed;
		}
	};
}


#endif