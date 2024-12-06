#include "GLBLoader.h"

#include "Material.h"
#include "Mesh.h"
#include "Node.h"
#include "Texture.h"

#include <tiny_gltf.h>

namespace JLEngine
{
    GLBLoader::GLBLoader()
    {
        
    }

	Node* GLBLoader::LoadGLB(const std::string& fileName)
	{
		tinygltf::TinyGLTF loader;
		tinygltf::Model model;
		std::string err;
		std::string warn;

		bool ret = loader.LoadBinaryFromFile(&model, &err, &warn, fileName);
		if (!warn.empty())
		{
			std::cerr << "Warning: " << warn << std::endl;
		}
		if (!ret)
		{
			std::cerr << "Error: " << err << std::endl;
			return nullptr;
		}
		if (model.scenes.empty())
		{
			std::cerr << "Error: No scenes in the GLB file." << std::endl;
			return nullptr;
		}

		const tinygltf::Scene& scene = model.scenes[model.defaultScene];
		Node* rootNode = new Node("RootNode");

		// Parse nodes
		for (int nodeIndex : scene.nodes)
		{
			const tinygltf::Node& gltfNode = model.nodes[nodeIndex];
			auto childNode = ParseNode(model, gltfNode);
			if (childNode)
			{
				rootNode->AddChild(childNode);
			}
		}
	}

	std::shared_ptr<Node> GLBLoader::ParseNode(const tinygltf::Model& model, const tinygltf::Node& gltfNode)
	{
		auto node = std::make_shared<Node>(gltfNode.name.empty() ? "UnnamedNode" : gltfNode.name);

		if (gltfNode.mesh >= 0)
		{
			node->SetTag(NodeTag::Mesh);
		}
		else if (gltfNode.camera >= 0)
		{
			node->SetTag(NodeTag::Camera);
		}
		else if (gltfNode.light >= 0)
		{
			node->SetTag(NodeTag::Light);
		}
		else
		{
			node->SetTag(NodeTag::Default);
		}

		if (!gltfNode.matrix.empty())
		{
			glm::mat4 matrix(1.0f);
			std::memcpy(&matrix, gltfNode.matrix.data(), sizeof(float) * 16);
			node->localMatrix = matrix;
		}
		else
		{
			// Parse transformations
			if (!gltfNode.translation.empty())
			{
				node->translation = glm::vec3(
					gltfNode.translation[0],
					gltfNode.translation[1],
					gltfNode.translation[2]
				);
			}

			if (!gltfNode.rotation.empty())
			{
				node->rotation = glm::quat(
					gltfNode.rotation[3],
					gltfNode.rotation[0],
					gltfNode.rotation[1],
					gltfNode.rotation[2]
				);
			}

			if (!gltfNode.scale.empty())
			{
				node->scale = glm::vec3(
					gltfNode.scale[0],
					gltfNode.scale[1],
					gltfNode.scale[2]
				);
			}
		}

		// Parse mesh
		if (gltfNode.mesh >= 0)
		{
			const tinygltf::Mesh& gltfMesh = model.meshes[gltfNode.mesh];
			auto mesh = ParseMesh(model, gltfMesh, gltfNode.mesh);
			if (mesh)
			{
				
			}
		}

		// Parse child nodes
		for (int childIndex : gltfNode.children)
		{
			const tinygltf::Node& childGltfNode = model.nodes[childIndex];
			auto childNode = ParseNode(model, childGltfNode);
			if (childNode)
			{
				node->AddChild(childNode);
			}
		}

		return node;
	}

	Mesh* GLBLoader::ParseMesh(const tinygltf::Model& model, const tinygltf::Mesh& gltfMesh, int meshIndex)
	{
		auto it = meshCache.find(meshIndex);
		if (it != meshCache.end())
		{
			std::cout << "Using cached mesh for index: " << meshIndex << std::endl;
			return it->second;
		}

		// Parse and cache the mesh
		auto mesh = new Mesh(0, gltfMesh.name.empty() ? "UnnamedMesh" : gltfMesh.name);

		// Parse primitives (placeholder for now)
		for (const tinygltf::Primitive& gltfPrimitive : gltfMesh.primitives)
		{
			//gltfPrimitive.indices
			//auto primitive = ParsePrimitive(model, gltfPrimitive);
			//if (primitive)
			//{
			//	mesh->AddPrimitive(primitive);
			//}
		}

		// Add to cache
		meshCache[meshIndex] = mesh;
		return mesh;
	}

	Material* GLBLoader::ParseMaterial(tinygltf::Model& model, tinygltf::Material& gltfMaterial, int matIdx)
	{
		auto it = materialCache.find(matIdx);
		if (it != materialCache.end())
		{
			std::cout << "Using cached material for index: " << matIdx << std::endl;
			return it->second;
		}

		auto material = new Material(0, gltfMaterial.name.empty() ? "UnnamedMat" : gltfMaterial.name);
		int texId = 0;
		if (gltfMaterial.values.find("baseColorFactor") != gltfMaterial.values.end())
		{
			const auto& factor = gltfMaterial.values.at("baseColorFactor").ColorFactor();
			material->baseColorFactor = glm::vec4(factor[0], factor[1], factor[2], factor[3]);
		}
		else
		{
			material->baseColorFactor = glm::vec4(1.0f);
		}

		// Base color texture
		if (gltfMaterial.values.find("baseColorTexture") != gltfMaterial.values.end())
		{
			int textureIndex = gltfMaterial.values.at("baseColorTexture").TextureIndex();
			auto texName = gltfMaterial.name + std::to_string(texId++);
			material->baseColorTexture = ParseTexture(model, texName, textureIndex);
		}

		// Metallic and roughness factors
		if (gltfMaterial.values.find("metallicFactor") != gltfMaterial.values.end())
		{
			material->metallicFactor = static_cast<float>(gltfMaterial.values.at("metallicFactor").Factor());
		}
		else
		{
			material->metallicFactor = 1.0f;
		}

		if (gltfMaterial.values.find("roughnessFactor") != gltfMaterial.values.end())
		{
			material->roughnessFactor = static_cast<float>(gltfMaterial.values.at("roughnessFactor").Factor());
		}
		else
		{
			material->roughnessFactor = 1.0f;
		}

		// Metallic-roughness texture
		if (gltfMaterial.values.find("metallicRoughnessTexture") != gltfMaterial.values.end())
		{
			int textureIndex = gltfMaterial.values.at("metallicRoughnessTexture").TextureIndex();
			auto texName = gltfMaterial.name + std::to_string(texId++);
			material->metallicRoughnessTexture = ParseTexture(model, texName, textureIndex);
		}

		// Normal map
		if (gltfMaterial.additionalValues.find("normalTexture") != gltfMaterial.additionalValues.end())
		{
			int textureIndex = gltfMaterial.additionalValues.at("normalTexture").TextureIndex();
			auto texName = gltfMaterial.name + std::to_string(texId++);
			material->normalTexture = ParseTexture(model, texName, textureIndex);
		}

		// Occlusion map
		if (gltfMaterial.additionalValues.find("occlusionTexture") != gltfMaterial.additionalValues.end())
		{
			int textureIndex = gltfMaterial.additionalValues.at("occlusionTexture").TextureIndex();
			auto texName = gltfMaterial.name + std::to_string(texId++);
			material->occlusionTexture = ParseTexture(model, texName, textureIndex);
		}

		// Emissive map
		if (gltfMaterial.additionalValues.find("emissiveTexture") != gltfMaterial.additionalValues.end())
		{
			int textureIndex = gltfMaterial.additionalValues.at("emissiveTexture").TextureIndex();
			auto texName = gltfMaterial.name + std::to_string(texId++);
			material->emissiveTexture = ParseTexture(model, texName, textureIndex);
		}

		// Emissive factor
		if (gltfMaterial.additionalValues.find("emissiveFactor") != gltfMaterial.additionalValues.end())
		{
			const auto& factor = gltfMaterial.additionalValues.at("emissiveFactor").ColorFactor();
			material->emissiveFactor = glm::vec3(factor[0], factor[1], factor[2]);
		}
		else
		{
			material->emissiveFactor = glm::vec3(0.0f);
		}

		// Alpha properties
		material->alphaMode = gltfMaterial.alphaMode;
		material->alphaCutoff = static_cast<float>(gltfMaterial.alphaCutoff);
		material->doubleSided = gltfMaterial.doubleSided;

		// Add to cache
		materialCache[matIdx] = material;
		return material;
	}

	Texture* GLBLoader::ParseTexture(const tinygltf::Model& model, std::string& name, int textureIndex)
	{
		if (textureIndex < 0 || textureIndex >= model.textures.size())
			return nullptr;

		const auto& texture = model.textures[textureIndex];
		const auto& imageData = model.images[texture.source];

		const std::string& finalName = name + (texture.name.empty() ? "UnnamedTexture" : texture.name);
		uint32 width = static_cast<uint32>(imageData.width);
		uint32 height = static_cast<uint32>(imageData.height);
		int channels = imageData.component;
		std::vector<unsigned char> data = imageData.image; // Raw pixel data

		auto jltexture = new Texture(0, name, width, height, data, channels);
		textureCache[textureIndex] = jltexture;

		return jltexture;
	}

	glm::vec4 GLBLoader::GetVec4FromValue(const tinygltf::Value& value, const glm::vec4& defaultValue)
	{
		if (!value.IsArray() || value.ArrayLen() != 4)
		{
			return defaultValue; // Return default if the value is not a valid vec4
		}

		return glm::vec4(
			static_cast<float>(value.Get(0).IsNumber() ? value.Get(0).Get<double>() : defaultValue.x),
			static_cast<float>(value.Get(1).IsNumber() ? value.Get(1).Get<double>() : defaultValue.y),
			static_cast<float>(value.Get(2).IsNumber() ? value.Get(2).Get<double>() : defaultValue.z),
			static_cast<float>(value.Get(3).IsNumber() ? value.Get(3).Get<double>() : defaultValue.w));
	}

	glm::vec3 GLBLoader::GetVec3FromValue(const tinygltf::Value& value, const glm::vec3& defaultValue)
	{
		if (!value.IsArray() || value.ArrayLen() != 3)
		{
			return defaultValue;
		}

		return glm::vec3(
			static_cast<float>(value.Get(0).IsNumber() ? value.Get(0).Get<double>() : defaultValue.x),
			static_cast<float>(value.Get(1).IsNumber() ? value.Get(1).Get<double>() : defaultValue.y),
			static_cast<float>(value.Get(2).IsNumber() ? value.Get(2).Get<double>() : defaultValue.z));
	}

	//JLEngine::Material* GLBLoader::LoadMaterial(const tinygltf::Model& model, const tinygltf::Material& gltfMaterial, MaterialManager* matMgr, TextureManager* textureMgr)
	//{
	//	auto material = matMgr->CreateMaterial(gltfMaterial.name);
	//	int texId = 0;
	//	if (gltfMaterial.values.find("baseColorFactor") != gltfMaterial.values.end())
	//	{
	//		const auto& factor = gltfMaterial.values.at("baseColorFactor").ColorFactor();
	//		material->baseColorFactor = glm::vec4(factor[0], factor[1], factor[2], factor[3]);
	//	}
	//	else
	//	{
	//		material->baseColorFactor = glm::vec4(1.0f);
	//	}
	//
	//	// Base color texture
	//	if (gltfMaterial.values.find("baseColorTexture") != gltfMaterial.values.end())
	//	{
	//		int textureIndex = gltfMaterial.values.at("baseColorTexture").TextureIndex();
	//		auto texName = gltfMaterial.name + std::to_string(texId++);
	//		material->baseColorTexture = LoadTexture(model, texName, textureIndex, textureMgr);
	//	}
	//
	//	// Metallic and roughness factors
	//	if (gltfMaterial.values.find("metallicFactor") != gltfMaterial.values.end())
	//	{
	//		material->metallicFactor = static_cast<float>(gltfMaterial.values.at("metallicFactor").Factor());
	//	}
	//	else
	//	{
	//		material->metallicFactor = 1.0f;
	//	}
	//
	//	if (gltfMaterial.values.find("roughnessFactor") != gltfMaterial.values.end())
	//	{
	//		material->roughnessFactor = static_cast<float>(gltfMaterial.values.at("roughnessFactor").Factor());
	//	}
	//	else
	//	{
	//		material->roughnessFactor = 1.0f;
	//	}
	//
	//	// Metallic-roughness texture
	//	if (gltfMaterial.values.find("metallicRoughnessTexture") != gltfMaterial.values.end())
	//	{
	//		int textureIndex = gltfMaterial.values.at("metallicRoughnessTexture").TextureIndex();
	//		auto texName = gltfMaterial.name + std::to_string(texId++);
	//		material->metallicRoughnessTexture = LoadTexture(model, texName, textureIndex, textureMgr);
	//	}
	//
	//	// Normal map
	//	if (gltfMaterial.additionalValues.find("normalTexture") != gltfMaterial.additionalValues.end())
	//	{
	//		int textureIndex = gltfMaterial.additionalValues.at("normalTexture").TextureIndex();
	//		auto texName = gltfMaterial.name + std::to_string(texId++);
	//		material->normalTexture = LoadTexture(model, texName, textureIndex, textureMgr);
	//	}
	//
	//	// Occlusion map
	//	if (gltfMaterial.additionalValues.find("occlusionTexture") != gltfMaterial.additionalValues.end())
	//	{
	//		int textureIndex = gltfMaterial.additionalValues.at("occlusionTexture").TextureIndex();
	//		auto texName = gltfMaterial.name + std::to_string(texId++);
	//		material->occlusionTexture = LoadTexture(model, texName, textureIndex, textureMgr);
	//	}
	//
	//	// Emissive map
	//	if (gltfMaterial.additionalValues.find("emissiveTexture") != gltfMaterial.additionalValues.end())
	//	{
	//		int textureIndex = gltfMaterial.additionalValues.at("emissiveTexture").TextureIndex();
	//		auto texName = gltfMaterial.name + std::to_string(texId++);
	//		material->emissiveTexture = LoadTexture(model, texName, textureIndex, textureMgr);
	//	}
	//
	//	// Emissive factor
	//	if (gltfMaterial.additionalValues.find("emissiveFactor") != gltfMaterial.additionalValues.end())
	//	{
	//		const auto& factor = gltfMaterial.additionalValues.at("emissiveFactor").ColorFactor();
	//		material->emissiveFactor = glm::vec3(factor[0], factor[1], factor[2]);
	//	}
	//	else
	//	{
	//		material->emissiveFactor = glm::vec3(0.0f);
	//	}
	//
	//	// Alpha properties
	//	material->alphaMode = gltfMaterial.alphaMode;
	//	material->alphaCutoff = static_cast<float>(gltfMaterial.alphaCutoff);
	//	material->doubleSided = gltfMaterial.doubleSided;
	//
	//	// Specular-glossiness workflow
	//	if (gltfMaterial.extensions.find("KHR_materials_pbrSpecularGlossiness") != gltfMaterial.extensions.end())
	//	{
	//		material->usesSpecularGlossinessWorkflow = true;
	//
	//		const auto& extension = gltfMaterial.extensions.at("KHR_materials_pbrSpecularGlossiness");
	//
	//		if (extension.Has("diffuseFactor"))
	//		{
	//			const auto& factor = extension.Get("diffuseFactor");
	//			material->diffuseFactor = GetVec4FromValue(factor, glm::vec4(1.0f));
	//		}
	//
	//		if (extension.Has("diffuseTexture"))
	//		{
	//			const tinygltf::Value& textureValue = extension.Get("diffuseTexture");
	//			if (textureValue.Has("index"))
	//			{
	//				int textureIndex = textureValue.Get("index").Get<int>();
	//				auto texName = gltfMaterial.name + std::to_string(texId++);
	//				material->diffuseTexture = LoadTexture(model, texName, textureIndex, textureMgr);
	//			}
	//		}
	//
	//		if (extension.Has("specularFactor"))
	//		{
	//			const tinygltf::Value& factor = extension.Get("specularFactor");
	//			material->specularFactor = GetVec3FromValue(factor, glm::vec3(1.0f));
	//		}
	//
	//		if (extension.Has("specularGlossinessTexture"))
	//		{
	//			const tinygltf::Value& textureValue = extension.Get("specularGlossinessTexture");
	//			if (textureValue.Has("index"))
	//			{
	//				int textureIndex = textureValue.Get("index").Get<int>();
	//				auto texName = gltfMaterial.name + std::to_string(texId++);
	//				material->specularGlossinessTexture = LoadTexture(model, texName, textureIndex, textureMgr);
	//			}
	//		}
	//	}
	//	return material;
	//}
}