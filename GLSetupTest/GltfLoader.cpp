#include "GltfLoader.h"
#include "Geometry.h"
#include "AssetLoader.h"

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

	JLEngine::Mesh* PrimitiveFromMesh(const tinygltf::Model& model, std::string& name, const tinygltf::Primitive& primitive, MeshManager* meshMgr, AssetGenerationSettings& settings)
	{
		if (primitive.mode != 4)
		{
			std::cout << "Primitive not using triangles!" << std::endl;
		}

		// Create a VertexBuffer
		JLEngine::VertexBuffer vbo(GL_ARRAY_BUFFER, GL_FLOAT, GL_STATIC_DRAW);
		std::vector<float> positions;
		std::vector<float> normals;
		std::vector<float> texCoords;
		std::vector<float> tangents;
		std::vector<float> vertexData;

		LoadPositionAttribute(model, primitive, positions);
		bool hasNormals = LoadNormalAttribute(model, primitive, normals);
		bool hasUVs = LoadTexCoordAttribute(model, primitive, texCoords);
		bool hasTangents = LoadTangentAttribute(model, primitive, tangents);

		if (!hasUVs)
		{
			std::cerr << "Primitive doesn't have UV coordinates" << std::endl;
			texCoords.insert(texCoords.end(), (positions.size() / 3) * 2, 0.0f);
			tangents.insert(tangents.end(), (positions.size() / 3) * 4, 0.0f);
			tangents.insert(tangents.end(), (positions.size() / 3) * 4, 0.0f);
			for (size_t i = 0; i < positions.size() / 3; ++i)
				tangents[i * 4 + 3] = 1.0f; 
			hasTangents = true;
			hasUVs = true;
		}

		// Process IndexBuffer
		JLEngine::IndexBuffer ibo(GL_ELEMENT_ARRAY_BUFFER, GL_UNSIGNED_INT, GL_STATIC_DRAW);
		bool hasIndices = LoadIndices(model, primitive, ibo);

		if (!hasNormals && settings.GenerateNormals)
		{
			std::cout << "Generating normals..." << std::endl;
			if (settings.NormalGenType == NormalGen::Flat)
			{
				if (hasIndices)
					normals = Geometry::CalculateFlatNormals(positions, ibo.GetBuffer());
				else
					normals = Geometry::CalculateFlatNormals(positions);
			}
			else
			{
				if (hasIndices)
					normals = Geometry::CalculateSmoothNormals(positions, ibo.GetBuffer());
				else
					normals = Geometry::CalculateSmoothNormals(positions);
			}
			hasNormals = true;
		}
		if (!hasTangents && hasNormals && hasUVs && settings.GenerateTangents)
		{
			std::cout << "Generating tangents..." << std::endl;

			if (hasIndices)
				tangents = Geometry::CalculateTangents(positions, normals, texCoords, ibo.GetBuffer());
			else
				tangents = Geometry::CalculateTangents(positions, normals, texCoords);

			hasTangents = true;
		}

		if (hasTangents)
		{
			Geometry::GenerateInterleavedVertexData(positions, normals, texCoords, tangents, vertexData);
		}
		else
		{
			Geometry::GenerateInterleavedVertexData(positions, normals, texCoords, vertexData);
		}
		vbo.Set(vertexData);

		int offset = 0;
		std::set<JLEngine::VertexAttribute> attributes;
		attributes.insert(JLEngine::VertexAttribute(JLEngine::POSITION, 0, 3));
		offset += sizeof(float) * 3;

		if (hasNormals)
		{			
			attributes.insert(JLEngine::VertexAttribute(JLEngine::NORMAL, offset, 3));
			offset += sizeof(float) * 3;
		}
		if (hasUVs)
		{			
			attributes.insert(JLEngine::VertexAttribute(JLEngine::TEX_COORD_2D, offset, 2));
			offset += sizeof(float) * 2;
		}
		if (hasTangents)
		{
			attributes.insert(JLEngine::VertexAttribute(JLEngine::TANGENT, offset, 3));
		}

		for (VertexAttribute attrib : attributes)
		{
			vbo.AddAttribute(attrib);
		}
		vbo.CalcStride();

		JLEngine::Mesh* jlmesh = nullptr;
		if (!hasIndices)
		{
			jlmesh = meshMgr->LoadMeshFromData(name, vbo);
		}
		else
		{
			jlmesh = meshMgr->LoadMeshFromData(name, vbo, ibo);
		}
		return jlmesh;
	}

	JLEngine::Mesh* MergePrimitivesToMesh(const tinygltf::Model& model, std::string& name, const tinygltf::Mesh& mesh, MeshManager* meshMgr, AssetGenerationSettings& settings)
	{
		std::vector<float> positions, normals, texCoords, tangents, vertexData;
		std::vector<IndexBuffer> ibos;
		size_t vertexOffset = positions.size() / 3;
		size_t indexOffset = 0;

		// Load attributes for all primitives
		for (const auto& primitive : mesh.primitives)
		{
			if (primitive.mode != 4)
			{
				std::cout << "Primitive not using triangles!" << std::endl;
			}

			size_t initialVertexCount = positions.size() / 3;
			LoadPositionAttribute(model, primitive, positions);
			bool hasNormals = LoadNormalAttribute(model, primitive, normals);
			bool hasUVs = LoadTexCoordAttribute(model, primitive, texCoords);
			bool hasTangents = LoadTangentAttribute(model, primitive, tangents);

			size_t newVertexCount = positions.size() / 3;
			size_t addedVertexCount = newVertexCount - initialVertexCount;

			JLEngine::IndexBuffer ibo(GL_ELEMENT_ARRAY_BUFFER, GL_UNSIGNED_INT, GL_STATIC_DRAW);
			bool hasIndices = LoadIndices(model, primitive, ibo);
			if (hasIndices)
			{ 
				auto indices = ibo.GetBuffer();
				for (auto& index : indices) {
					index += (unsigned int)indexOffset;
				}
				ibo.Set(indices);
				ibos.push_back(ibo);
			}
			indexOffset += addedVertexCount;

			if (!hasNormals && settings.GenerateNormals)
			{
				std::cout << "Generating normals..." << std::endl;
				if (settings.NormalGenType == NormalGen::Flat)
				{
					if (hasIndices)
						normals = Geometry::CalculateFlatNormals(positions, ibo.GetBuffer());
					else
						normals = Geometry::CalculateFlatNormals(positions);
				}
				else
				{
					if (hasIndices)
						normals = Geometry::CalculateSmoothNormals(positions, ibo.GetBuffer());
					else
						normals = Geometry::CalculateSmoothNormals(positions);
				}
				hasNormals = true;
			}
			if (!hasTangents && hasNormals && hasUVs && settings.GenerateTangents)
			{
				std::cout << "Generating tangents..." << std::endl;

				if (hasIndices)
					tangents = Geometry::CalculateTangents(positions, normals, texCoords, ibo.GetBuffer());
				else
					tangents = Geometry::CalculateTangents(positions, normals, texCoords);

				hasTangents = true;
			}

			// Pad normals if missing
			if (!hasNormals) { normals.insert(normals.end(), addedVertexCount * 3, 1.0f); }
			if (!hasUVs) { texCoords.insert(texCoords.end(), addedVertexCount * 2, 0.0f); }
			if (!hasTangents) { tangents.insert(tangents.end(), addedVertexCount * 4, 0.0f); }
		}

		if (normals.size() > 0 && normals.size() / 3 != positions.size() / 3) 
		{
			std::cerr << "Error: Normal data size does not match position data size in mesh: " << name << std::endl;
		}
		if (texCoords.size() > 0 && texCoords.size() / 2 != positions.size() / 3) 
		{
			std::cerr << "Error: TexCoord data size does not match position data size in mesh: " << name << std::endl;
		}
		if (tangents.size() > 0 && tangents.size() / 3 != positions.size() / 3) 
		{
			std::cerr << "Error: Tangent data size does not match position data size in mesh: " << name << std::endl;
		}

		// Generate interleaved vertex data based on whether tangents are available
		if (tangents.size() > 0)
		{
			Geometry::GenerateInterleavedVertexData(positions, normals, texCoords, tangents, vertexData);
		}
		else 
		{
			std::cerr << "Warning: Tangents not found in some or all primitives. Generating vertex data without tangents." << std::endl;
			Geometry::GenerateInterleavedVertexData(positions, normals, texCoords, vertexData);
		}

		// Create the shared VertexBuffer
		JLEngine::VertexBuffer vbo(GL_ARRAY_BUFFER, GL_FLOAT, GL_STATIC_DRAW);
		vbo.Set(vertexData);

		// Define the vertex attributes
		int offset = 0;
		std::set<JLEngine::VertexAttribute> attributes;
		attributes.insert(JLEngine::VertexAttribute(JLEngine::POSITION, 0, 3));
		offset += sizeof(float) * 3;

		if (!normals.empty()) { attributes.insert(JLEngine::VertexAttribute(JLEngine::NORMAL, offset, 3)); offset += sizeof(float) * 3; }
		if (!texCoords.empty()) { attributes.insert(JLEngine::VertexAttribute(JLEngine::TEX_COORD_2D, offset, 2)); offset += sizeof(float) * 2; }
		if (!tangents.empty()) { attributes.insert(JLEngine::VertexAttribute(JLEngine::TANGENT, offset, 4)); }

		for (auto attrib : attributes) 
		{
			vbo.AddAttribute(attrib);
		}
		vbo.CalcStride();

		auto jlmesh = meshMgr->LoadMeshFromData(name, vbo, ibos);
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
	bool LoadNormalAttribute(const tinygltf::Model& model, const tinygltf::Primitive& primitive,
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
				return false;
			}

			size_t stride = normalAccessor.ByteStride(model.bufferViews[normalAccessor.bufferView]);
			if (stride != sizeof(float) * 3)
			{
				std::cerr << "Error: Unsupported NORMAL stride: " << stride << std::endl;
				return false;
			}

			// Validate buffer bounds
			size_t requiredSize = normalBufferView.byteOffset + normalAccessor.byteOffset +
				normalAccessor.count * 3 * sizeof(float);
			if (requiredSize > normalBuffer.data.size()) 
			{
				std::cerr << "Error: Normal accessor exceeds buffer bounds!" << std::endl;
				return false;
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
			return false;
		}
		return true;
	}

	// Function to load TEXCOORD_0 attribute
	bool LoadTexCoordAttribute(const tinygltf::Model& model, const tinygltf::Primitive& primitive,
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
				return false;
			}

			size_t stride = texCoordAccessor.ByteStride(model.bufferViews[texCoordAccessor.bufferView]);
			if (stride != sizeof(float) * 2)
			{
				std::cerr << "Error: Unsupported TEXCOORD_0 stride: " << stride << std::endl;
			}

			// Validate buffer bounds
			size_t requiredSize = texCoordBufferView.byteOffset + texCoordAccessor.byteOffset +
				texCoordAccessor.count * 2 * sizeof(float);
			if (requiredSize > texCoordBuffer.data.size()) {
				std::cerr << "Error: TEXCOORD_0 accessor exceeds buffer bounds!" << std::endl;
				return false;
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
			return false;
		}
		return true;
	}

	bool LoadTangentAttribute(const tinygltf::Model& model, const tinygltf::Primitive& primitive, std::vector<float>& tangentData)
	{
		const auto& tangentAttr = primitive.attributes.find("TANGENT");
		if (tangentAttr != primitive.attributes.end())
		{
			const tinygltf::Accessor& tangentAccessor = model.accessors[tangentAttr->second];
			const tinygltf::BufferView& tangentBufferView = model.bufferViews[tangentAccessor.bufferView];
			const tinygltf::Buffer& tangentBuffer = model.buffers[tangentBufferView.buffer];

			// Validate component type
			if (tangentAccessor.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT)
			{
				std::cerr << "Error: Unsupported TANGENT component type: " << tangentAccessor.componentType << std::endl;
				return false;
			}

			size_t stride = tangentAccessor.ByteStride(model.bufferViews[tangentAccessor.bufferView]);
			if (stride != sizeof(float) * 4)
			{
				std::cerr << "Error: Unsupported TANGENT stride: " << stride << std::endl;
				return false;
			}

			// Validate buffer bounds
			size_t requiredSize = tangentBufferView.byteOffset + tangentAccessor.byteOffset +
				tangentAccessor.count * 4 * sizeof(float); // Assuming vec3 tangents
			if (requiredSize > tangentBuffer.data.size())
			{
				std::cerr << "Error: Tangent accessor exceeds buffer bounds!" << std::endl;
				return false;
			}

			const float* tangents = reinterpret_cast<const float*>(
				&tangentBuffer.data[tangentBufferView.byteOffset + tangentAccessor.byteOffset]);

			size_t count = tangentAccessor.count * 4; 
			tangentData.insert(tangentData.end(), tangents, tangents + count);
		}
		else
		{			
			std::cerr << "Warning: TANGENT attribute not found in primitive" << std::endl;
			return false;
		}
		return true;
	}

	// Function to load indices
	bool LoadIndices(const tinygltf::Model& model, const tinygltf::Primitive& primitive, JLEngine::IndexBuffer& ibo)
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
				return false;
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
					auto msg = "Unsupported index component type: " + std::to_string(indexAccessor.componentType);
					throw std::exception(msg.c_str());
					return false;
			}

			ibo.Set(indexVector);
			return true;
		}
		return false;
	}
}
