#include "GltfLoader.h"
#include "Geometry.h"

#undef APIENTRY
#define TINYGLTF_IMPLEMENTATION
#include <tiny_gltf.h>

#include "json.hpp"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <iostream>
#include <vector>
#include "MeshManager.h"

namespace JLEngine
{
	JLEngine::Mesh* JLEngine::LoadModelGLB(std::string fileName, Graphics* graphics)
	{
		JLEngine::Mesh* jlmesh = new Mesh(0, fileName);
		tinygltf::TinyGLTF loader;
		tinygltf::Model model;
		std::string err;
		std::string warn;

		bool ret = loader.LoadBinaryFromFile(&model, &err, &warn, fileName);
		if (!ret)
		{
			std::cout << err << std::endl;
			return nullptr;
		}

		for (const auto& mesh : model.meshes) 
		{ 
			for (const auto& primitive : mesh.primitives) 
			{ 
				// Create a VertexBuffer
				JLEngine::VertexBuffer vbo(GL_ARRAY_BUFFER, GL_FLOAT, GL_STATIC_DRAW);
				std::vector<float> positions;
				std::vector<float> normals;
				std::vector<float> texCoords;
				std::vector<float> vertexData;

				LoadPositionAttribute(model, primitive, positions);
				LoadNormalAttribute(model, primitive, normals);
				LoadTexCoordAttribute(model, primitive, texCoords);
				Geometry::GenerateInterleavedVertexData(positions, normals, texCoords, vertexData);
				vbo.Set(vertexData);

				std::set<JLEngine::VertexAttribute> attributes;
				attributes.insert(JLEngine::VertexAttribute(JLEngine::POSITION, 0, 3));
				attributes.insert(JLEngine::VertexAttribute(JLEngine::NORMAL, sizeof(float) * 3, 3));
				attributes.insert(JLEngine::VertexAttribute(JLEngine::TEX_COORD_2D, sizeof(float) * 6, 2));

				for (VertexAttribute attrib : attributes) 
				{
					vbo.AddAttribute(attrib);
				}
				vbo.CalcStride();

				// Process IndexBuffer
				JLEngine::IndexBuffer ibo(GL_ELEMENT_ARRAY_BUFFER, GL_UNSIGNED_INT, GL_STATIC_DRAW);
				LoadIndices(model, primitive, ibo);

				// Create and initialize the Mesh
				jlmesh->SetVertexBuffer(vbo);
				if (primitive.indices >= 0) 
				{
					jlmesh->AddIndexBuffer(ibo);
				}

				jlmesh->UploadToGPU(graphics, true);
			}
		}
		return jlmesh;
	}

	JLEngine::Mesh* PrimitiveFromMesh(const tinygltf::Model& model, std::string& name, const tinygltf::Primitive& primitive, MeshManager* meshMgr)
	{
		// Create a VertexBuffer
		JLEngine::VertexBuffer vbo(GL_ARRAY_BUFFER, GL_FLOAT, GL_STATIC_DRAW);
		std::vector<float> positions;
		std::vector<float> normals;
		std::vector<float> texCoords;
		std::vector<float> vertexData;

		LoadPositionAttribute(model, primitive, positions);
		LoadNormalAttribute(model, primitive, normals);
		LoadTexCoordAttribute(model, primitive, texCoords);
		Geometry::GenerateInterleavedVertexData(positions, normals, texCoords, vertexData);
		vbo.Set(vertexData);

		std::set<JLEngine::VertexAttribute> attributes;
		attributes.insert(JLEngine::VertexAttribute(JLEngine::POSITION, 0, 3));
		attributes.insert(JLEngine::VertexAttribute(JLEngine::NORMAL, sizeof(float) * 3, 3));
		attributes.insert(JLEngine::VertexAttribute(JLEngine::TEX_COORD_2D, sizeof(float) * 6, 2));

		for (VertexAttribute attrib : attributes)
		{
			vbo.AddAttribute(attrib);
		}
		vbo.CalcStride();

		// Process IndexBuffer
		JLEngine::IndexBuffer ibo(GL_ELEMENT_ARRAY_BUFFER, GL_UNSIGNED_INT, GL_STATIC_DRAW);
		LoadIndices(model, primitive, ibo);

		// Create and initialize the Mesh
		auto jlmesh = meshMgr->LoadMeshFromData(name, vbo, ibo);
		return jlmesh;
	}

	JLEngine::Mesh* MergePrimitivesToMesh(const tinygltf::Model& model, std::string& name, const tinygltf::Mesh& mesh, MeshManager* meshMgr)
	{
		std::vector<float> positions, normals, texCoords, vertexData;
		for (const auto& primitive : mesh.primitives)
		{
			LoadPositionAttribute(model, primitive, positions);
			LoadNormalAttribute(model, primitive, normals);
			LoadTexCoordAttribute(model, primitive, texCoords);
		}
		Geometry::GenerateInterleavedVertexData(positions, normals, texCoords, vertexData);

		// Create the shared VertexBuffer
		JLEngine::VertexBuffer vbo(GL_ARRAY_BUFFER, GL_FLOAT, GL_STATIC_DRAW);
		vbo.Set(vertexData);

		// Define the vertex attributes
		std::set<JLEngine::VertexAttribute> attributes;
		attributes.insert(JLEngine::VertexAttribute(JLEngine::POSITION, 0, 3));
		attributes.insert(JLEngine::VertexAttribute(JLEngine::NORMAL, sizeof(float) * 3, 3));
		attributes.insert(JLEngine::VertexAttribute(JLEngine::TEX_COORD_2D, sizeof(float) * 6, 2));

		for (VertexAttribute attrib : attributes)
		{
			vbo.AddAttribute(attrib);
		}
		vbo.CalcStride();

		// Step 2: Create a Mesh instance
		auto jlmesh = meshMgr->CreateEmptyMesh(name);
		jlmesh->SetVertexBuffer(vbo);

		// Step 3: Create separate index buffers for each primitive
		for (size_t i = 0; i < mesh.primitives.size(); ++i)
		{
			const auto& primitive = mesh.primitives[i];

			JLEngine::IndexBuffer ibo(GL_ELEMENT_ARRAY_BUFFER, GL_UNSIGNED_INT, GL_STATIC_DRAW);
			LoadIndices(model, primitive, ibo);
			jlmesh->AddIndexBuffer(ibo);
		}

		return jlmesh;
	}

	JLEngine::Material* LoadMaterial(const tinygltf::Model& model, const tinygltf::Material& gltfMaterial, MaterialManager* matMgr, TextureManager* textureMgr)
	{
		auto material = matMgr->CreateMaterial(gltfMaterial.name);
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
			material->baseColorTexture = LoadTexture(model, texName, textureIndex, textureMgr);
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
			material->metallicRoughnessTexture = LoadTexture(model, texName, textureIndex, textureMgr);
		}

		// Normal map
		if (gltfMaterial.additionalValues.find("normalTexture") != gltfMaterial.additionalValues.end())
		{
			int textureIndex = gltfMaterial.additionalValues.at("normalTexture").TextureIndex();
			auto texName = gltfMaterial.name + std::to_string(texId++);
			material->normalTexture = LoadTexture(model, texName, textureIndex, textureMgr);
		}

		// Occlusion map
		if (gltfMaterial.additionalValues.find("occlusionTexture") != gltfMaterial.additionalValues.end())
		{
			int textureIndex = gltfMaterial.additionalValues.at("occlusionTexture").TextureIndex();
			auto texName = gltfMaterial.name + std::to_string(texId++);
			material->occlusionTexture = LoadTexture(model, texName, textureIndex, textureMgr);
		}

		// Emissive map
		if (gltfMaterial.additionalValues.find("emissiveTexture") != gltfMaterial.additionalValues.end())
		{
			int textureIndex = gltfMaterial.additionalValues.at("emissiveTexture").TextureIndex();
			auto texName = gltfMaterial.name + std::to_string(texId++);
			material->emissiveTexture = LoadTexture(model, texName, textureIndex, textureMgr);
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

		// Specular-glossiness workflow
		if (gltfMaterial.extensions.find("KHR_materials_pbrSpecularGlossiness") != gltfMaterial.extensions.end())
		{
			material->usesSpecularGlossinessWorkflow = true;

			const auto& extension = gltfMaterial.extensions.at("KHR_materials_pbrSpecularGlossiness");

			if (extension.Has("diffuseFactor"))
			{
				const auto& factor = extension.Get("diffuseFactor");
				material->diffuseFactor = GetVec4FromValue(factor, glm::vec4(1.0f));
			}

			if (extension.Has("diffuseTexture"))
			{
				const tinygltf::Value& textureValue = extension.Get("diffuseTexture");
				if (textureValue.Has("index"))
				{
					int textureIndex = textureValue.Get("index").Get<int>();
					auto texName = gltfMaterial.name + std::to_string(texId++);
					material->diffuseTexture = LoadTexture(model, texName, textureIndex, textureMgr);
				}
			}

			if (extension.Has("specularFactor"))
			{
				const tinygltf::Value& factor = extension.Get("specularFactor");
				material->specularFactor = GetVec3FromValue(factor, glm::vec3(1.0f));
			}

			if (extension.Has("specularGlossinessTexture"))
			{
				const tinygltf::Value& textureValue = extension.Get("specularGlossinessTexture");
				if (textureValue.Has("index"))
				{
					int textureIndex = textureValue.Get("index").Get<int>();
					auto texName = gltfMaterial.name + std::to_string(texId++);
					material->specularGlossinessTexture = LoadTexture(model, texName, textureIndex, textureMgr);
				}
			}
		}
		return material;
	}

	JLEngine::Texture* LoadTexture(const tinygltf::Model& model, std::string& name, int textureIndex, TextureManager* textureMgr)
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

		auto jltexture = textureMgr->CreateTextureFromData(finalName, width, height, channels, data);

		return jltexture;
	}


	// Function to extract a vec4 from a tinygltf::Value
	glm::vec4 GetVec4FromValue(const tinygltf::Value& value, const glm::vec4& defaultValue)
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

	glm::vec3 GetVec3FromValue(const tinygltf::Value& value, const glm::vec3& defaultValue)
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

	void LoadPositionAttribute(const tinygltf::Model& model, const tinygltf::Primitive& primitive,
		std::vector<float>& vertexData) 
	{
		const auto& positionAttr = primitive.attributes.find("POSITION");
		if (positionAttr != primitive.attributes.end()) 
		{
			const tinygltf::Accessor& positionAccessor = model.accessors[positionAttr->second];
			const tinygltf::BufferView& positionBufferView = model.bufferViews[positionAccessor.bufferView];
			const tinygltf::Buffer& positionBuffer = model.buffers[positionBufferView.buffer];

			// Validate component type
			if (positionAccessor.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT) 
			{
				std::cerr << "Error: Unsupported POSITION component type: " << positionAccessor.componentType << std::endl;
				return;
			}

			// Validate buffer bounds
			size_t requiredSize = positionBufferView.byteOffset + positionAccessor.byteOffset +
				positionAccessor.count * 3 * sizeof(float);
			if (requiredSize > positionBuffer.data.size()) 
			{
				std::cerr << "Error: Position accessor exceeds buffer bounds!" << std::endl;
				return;
			}

			const float* positions = reinterpret_cast<const float*>(
				&positionBuffer.data[positionBufferView.byteOffset + positionAccessor.byteOffset]);

			size_t count = positionAccessor.count * 3; // Assuming vec3 positions
			vertexData.insert(vertexData.end(), positions, positions + count);
		}
		else 
		{
			std::cerr << "Error: POSITION attribute not found in primitive!" << std::endl;
		}
	}

	// Function to load NORMAL attribute
	void LoadNormalAttribute(const tinygltf::Model& model, const tinygltf::Primitive& primitive,
		std::vector<float>& vertexData) 
	{
		const auto& normalAttr = primitive.attributes.find("NORMAL");
		if (normalAttr != primitive.attributes.end()) 
		{
			const tinygltf::Accessor& normalAccessor = model.accessors[normalAttr->second];
			const tinygltf::BufferView& normalBufferView = model.bufferViews[normalAccessor.bufferView];
			const tinygltf::Buffer& normalBuffer = model.buffers[normalBufferView.buffer];

			// Validate component type
			if (normalAccessor.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT) 
			{
				std::cerr << "Error: Unsupported NORMAL component type: " << normalAccessor.componentType << std::endl;
				return;
			}

			// Validate buffer bounds
			size_t requiredSize = normalBufferView.byteOffset + normalAccessor.byteOffset +
				normalAccessor.count * 3 * sizeof(float);
			if (requiredSize > normalBuffer.data.size()) 
			{
				std::cerr << "Error: Normal accessor exceeds buffer bounds!" << std::endl;
				return;
			}

			// Access the normals
			const float* normals = reinterpret_cast<const float*>(
				&normalBuffer.data[normalBufferView.byteOffset + normalAccessor.byteOffset]);

			size_t count = normalAccessor.count * 3; // Assuming vec3 normals
			vertexData.insert(vertexData.end(), normals, normals + count);
		}
		else 
		{
			std::cerr << "Warning: NORMAL attribute not found in primitive!" << std::endl;
		}
	}

	// Function to load TEXCOORD_0 attribute
	void LoadTexCoordAttribute(const tinygltf::Model& model, const tinygltf::Primitive& primitive,
		std::vector<float>& vertexData) 
	{
		const auto& texCoordAttr = primitive.attributes.find("TEXCOORD_0");
		if (texCoordAttr != primitive.attributes.end()) 
		{
			const tinygltf::Accessor& texCoordAccessor = model.accessors[texCoordAttr->second];
			const tinygltf::BufferView& texCoordBufferView = model.bufferViews[texCoordAccessor.bufferView];
			const tinygltf::Buffer& texCoordBuffer = model.buffers[texCoordBufferView.buffer];

			// Validate component type
			if (texCoordAccessor.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT) {
				std::cerr << "Error: Unsupported TEXCOORD_0 component type: " << texCoordAccessor.componentType << std::endl;
				return;
			}

			// Validate buffer bounds
			size_t requiredSize = texCoordBufferView.byteOffset + texCoordAccessor.byteOffset +
				texCoordAccessor.count * 2 * sizeof(float);
			if (requiredSize > texCoordBuffer.data.size()) {
				std::cerr << "Error: TEXCOORD_0 accessor exceeds buffer bounds!" << std::endl;
				return;
			}

			// Access the texture coordinates
			const float* texCoords = reinterpret_cast<const float*>(
				&texCoordBuffer.data[texCoordBufferView.byteOffset + texCoordAccessor.byteOffset]);

			size_t count = texCoordAccessor.count * 2; // Assuming vec2 texture coordinates
			vertexData.insert(vertexData.end(), texCoords, texCoords + count);
		}
		else 
		{
			std::cerr << "Warning: TEXCOORD_0 attribute not found in primitive!" << std::endl;
		}
	}

	// Function to load indices
	void LoadIndices(const tinygltf::Model& model, const tinygltf::Primitive& primitive, JLEngine::IndexBuffer& ibo) 
	{
		if (primitive.indices >= 0) 
		{
			const tinygltf::Accessor& indexAccessor = model.accessors[primitive.indices];
			const tinygltf::BufferView& indexBufferView = model.bufferViews[indexAccessor.bufferView];
			const tinygltf::Buffer& indexBuffer = model.buffers[indexBufferView.buffer];

			size_t startOffset = indexBufferView.byteOffset + indexAccessor.byteOffset;
			size_t endOffset = startOffset + indexAccessor.count * tinygltf::GetComponentSizeInBytes(indexAccessor.componentType);

			if (endOffset > indexBuffer.data.size()) 
			{
				std::cerr << "Index accessor out of buffer bounds!" << std::endl;
				return;
			}

			const void* data = &indexBuffer.data[indexBufferView.byteOffset + indexAccessor.byteOffset];
			std::vector<uint32_t> indexVector;

			switch (indexAccessor.componentType)
			{
				case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE: 
				{
					const uint8_t* indices = reinterpret_cast<const uint8_t*>(data);
					indexVector.assign(indices, indices + indexAccessor.count);
					break;
				}
				case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: 
				{
					const uint16_t* indices = reinterpret_cast<const uint16_t*>(data);
					indexVector.assign(indices, indices + indexAccessor.count);
					break;
				}
				case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT: 
				{
					const uint32_t* indices = reinterpret_cast<const uint32_t*>(data);
					indexVector.assign(indices, indices + indexAccessor.count);
					break;
				}
				default:
					std::cerr << "Unsupported index component type: " << indexAccessor.componentType << std::endl;
					return;
			}

			ibo.Set(indexVector);
		}
	}
}
