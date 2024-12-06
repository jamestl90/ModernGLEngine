#ifndef GLB_LOADER_H
#define GLB_LOADER_H

#include <glm/glm.hpp>
#include <tiny_gltf.h>
#include <string>
#include <iostream>

namespace JLEngine
{
	class Node;
	class Material;
	class Texture;
	class TextureManager;
	class MaterialManager;
	class Mesh;

	class GLBLoader
	{
	public:
		GLBLoader();

		Node* LoadGLB(const std::string& fileName);

	protected:
		std::shared_ptr<Node> ParseNode(const tinygltf::Model& model, const tinygltf::Node& gltfNode);
		Mesh* ParseMesh(const tinygltf::Model& model, const tinygltf::Mesh& gltfMesh, int meshIndex);
		Material* ParseMaterial(tinygltf::Model& model, tinygltf::Material& gltfMaterial, int matIdx);
		Texture* ParseTexture(const tinygltf::Model& model, std::string& name, int textureIndex);

		glm::vec4 GetVec4FromValue(const tinygltf::Value& value, const glm::vec4& defaultValue);
		glm::vec3 GetVec3FromValue(const tinygltf::Value& value, const glm::vec3& defaultValue);

		std::unordered_map<int, Mesh*> meshCache;
		std::unordered_map<int, Material*> materialCache;
		std::unordered_map<int, Texture*> textureCache;
	};
}

#endif