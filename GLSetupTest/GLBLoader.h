#ifndef GLB_LOADER_H
#define GLB_LOADER_H

#include <glm/glm.hpp>
#include <tiny_gltf.h>
#include <string>
#include <iostream>
#include <functional>
#include <unordered_map>

#include "VertexBuffers.h"
#include "Mesh.h"
#include "AnimData.h"

namespace JLEngine
{
	class Node;
	class Material;
	class Texture;
	class TextureManager;
	class MaterialManager;
	class Mesh;
	class Graphics;
	class ResourceLoader;

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
		GLBLoader(ResourceLoader* resourceLoader);

		std::shared_ptr<Node> LoadGLB(const std::string& fileName);

		std::unordered_map<VertexAttribKey, std::shared_ptr<VertexArrayObject>>& GetStaticVAOs() { return m_staticVAOs; }
		std::unordered_map<VertexAttribKey, std::shared_ptr<VertexArrayObject>>& GetDynamicVAOs() { return m_skinnedMeshVAOs; }
		std::unordered_map<VertexAttribKey, std::shared_ptr<VertexArrayObject>>& GetTransparentVAOs() { return m_transparentVAOs; }

		void ClearCaches();

	protected:
		std::shared_ptr<Node> ParseNode(const tinygltf::Model& model, const tinygltf::Node& gltfNode, int nodeIndex);
		std::shared_ptr<Mesh> ParseMesh(const tinygltf::Model& model, int meshIndex);
		std::shared_ptr<Animation> ParseAnimation(int animIdx, const tinygltf::Model& model, const tinygltf::Animation& gltfAnimation);
		void ParseSkin(const tinygltf::Model& model, const tinygltf::Skin& skin, Mesh& mesh);
		std::vector<float> GetKeyframeTimes(const tinygltf::Model& model, int accessorIndex);
		std::vector<glm::vec4> GetKeyframeValues(const tinygltf::Model& model, int accessorIndex);
		std::shared_ptr<Material> ParseMaterial(const tinygltf::Model& model, const tinygltf::Material& gltfMaterial, int matIdx);
		std::shared_ptr<Texture> ParseTexture(const tinygltf::Model& model, std::string& matName, const std::string& texName, int textureIndex, TexParams overwriteParams);
		void ParseTransform(std::shared_ptr<Node> node, const tinygltf::Node& gltfNode);
		void loadKHRTextureTransform(const tinygltf::Material& gltfMaterial, std::shared_ptr<Material> material);
		SubMesh CreateSubMeshAnim(const tinygltf::Model& model, const std::vector<const tinygltf::Primitive*>& primitives, MaterialVertexAttributeKey key);
		SubMesh CreateSubMesh(const tinygltf::Model& model, const std::vector<const tinygltf::Primitive*>& primitives, MaterialVertexAttributeKey key);
		void UpdateUVsFromScaleOffset(std::vector<float>& uvs, glm::vec2 scale, glm::vec2 offset);
		bool LoadIndices(const tinygltf::Model& model, const tinygltf::Primitive& primitive, std::vector<unsigned int>& indices);
		bool LoadTangentAttribute(const tinygltf::Model& model, const tinygltf::Primitive& primitive, std::vector<float>& tangentData);
		bool LoadTexCoordAttribute(const tinygltf::Model& model, const tinygltf::Primitive& primitive, std::vector<float>& vertexData);
		bool LoadTexCoord2Attribute(const tinygltf::Model& model, const tinygltf::Primitive& primitive, std::vector<float>& vertexData);
		bool LoadNormalAttribute(const tinygltf::Model& model, const tinygltf::Primitive& primitive, std::vector<float>& vertexData);
		void LoadPositionAttribute(const tinygltf::Model& model, const tinygltf::Primitive& primitive, std::vector<float>& vertexData);
		void LoadWeightAttribute(const tinygltf::Model& model, const tinygltf::Primitive& primitive, std::vector<float>& weights);
		void LoadJointAttribute(const tinygltf::Model& model, const tinygltf::Primitive& primitive, std::vector<uint16_t>& joints);
		void GenerateMissingAttributes(std::vector<float>& positions, 
			std::vector<float>& normals, 
			const std::vector<float>& texCoords, 
			std::vector<float>& tangents, 
			const std::vector<uint32_t>& indices, 
			VertexAttribKey key);
		void BatchLoadAttributes(const tinygltf::Model& model, 
			const std::vector<const tinygltf::Primitive*>& primitives, 
			std::vector<float>& positions, 
			std::vector<float>& normals, 
			std::vector<float>& texCoords, 
			std::vector<float>& texCoords2, 
			std::vector<float>& tangents, 
			std::vector<float>& weights,       
			std::vector<uint16_t>& joints,
			std::vector<uint32_t>& indices, 
			uint32_t& indexOffset, VertexAttribKey key);
		void LoadAttributes(const tinygltf::Model& model, 
			const tinygltf::Primitive* primitives, 
			std::vector<float>& positions, 
			std::vector<float>& normals, 
			std::vector<float>& texCoords, 
			std::vector<float>& tangents, 
			std::vector<uint32_t>& indices, 
			uint32_t& indexOffset);
		glm::vec4 GetVec4FromValue(const tinygltf::Value& value, const glm::vec4& defaultValue);
		glm::vec3 GetVec3FromValue(const tinygltf::Value& value, const glm::vec3& defaultValue);

		std::vector<Animation*> GetAnimationsFromSkeleton(int gltfSkeletonRoot);

	private:
		std::unordered_map<int, std::shared_ptr<Mesh>> meshCache;
		std::unordered_map<int, std::shared_ptr<Material>> materialCache;
		std::unordered_map<int, std::shared_ptr<Texture>> textureCache;
		std::unordered_map<int, std::vector<std::shared_ptr<Node>>> meshNodeReferences;

		std::unordered_map<VertexAttribKey, std::shared_ptr<VertexArrayObject>> m_staticVAOs;
		std::unordered_map<VertexAttribKey, std::shared_ptr<VertexArrayObject>> m_skinnedMeshVAOs;
		std::unordered_map<VertexAttribKey, std::shared_ptr<VertexArrayObject>> m_transparentVAOs;
		std::vector<Node*> m_nodeList;

		ResourceLoader* m_resourceLoader;
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