#include "GLBLoader.h"

#include "Material.h"
#include "Mesh.h"
#include "Node.h"
#include "Texture.h"
#include "Graphics.h"
#include "AssetLoader.h"

#include <tiny_gltf.h>
#include <glm/gtc/type_ptr.hpp>
#include "Geometry.h"

namespace JLEngine
{
    GLBLoader::GLBLoader(Graphics* graphics, AssetLoader* assetLoader) 
		: m_graphics(graphics), m_assetLoader(assetLoader)
    {
        
    }

	std::shared_ptr<Node> GLBLoader::LoadGLB(const std::string& fileName)
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
		auto rootNode = std::make_shared<Node>("RootNode");

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

		return rootNode;
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

		ParseTransform(node, gltfNode);

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

	Mesh* GLBLoader::ParseMesh(const tinygltf::Model& model, int meshIndex)
	{
		// Check if the mesh is already cached
		auto it = meshCache.find(meshIndex);
		if (it != meshCache.end())
		{
			return it->second; // Return cached mesh
		}

		const tinygltf::Mesh& gltfMesh = model.meshes[meshIndex];

		// Create a new Mesh
		auto mesh = new Mesh(gltfMesh.name.empty() ? "UnnamedMesh" : gltfMesh.name);

		// Group primitives by material and attributes
		std::unordered_map<MaterialAttributeKey, std::vector<const tinygltf::Primitive*>> groups;
		for (const auto& primitive : gltfMesh.primitives)
		{
			MaterialAttributeKey key(primitive.material, GetAttributesKey(primitive.attributes));
			groups[key].push_back(&primitive);
		}

		// Create batches for each group
		for (const auto& [key, primitives] : groups)
		{
			auto batch = CreateBatch(model, primitives, key);
			mesh->AddBatch(std::shared_ptr<Batch>(batch));
		}

		// Cache the mesh
		meshCache[meshIndex] = mesh;
		return mesh;
	}

	std::shared_ptr<Batch> GLBLoader::CreateBatch(const tinygltf::Model& model,
		const std::vector<const tinygltf::Primitive*>& primitives,
		MaterialAttributeKey key)
	{
		// Temporary data arrays
		std::vector<float> positions;
		std::vector<float> normals;
		std::vector<float> texCoords;
		std::vector<float> tangents;
		std::vector<uint32> indices;

		uint32_t indexOffset = 0;

		for (const auto* primitive : primitives)
		{
			// Load vertex attributes
			LoadPositionAttribute(model, *primitive, positions);
			LoadNormalAttribute(model, *primitive, normals);
			LoadTexCoordAttribute(model, *primitive, texCoords);
			LoadTangentAttribute(model, *primitive, tangents);

			// Check if the primitive has indices
			if (primitive->indices >= 0)
			{
				std::vector<unsigned int> tempIndices;
				if (LoadIndices(model, *primitive, tempIndices))
				{
					for (auto index : tempIndices)
					{
						indices.push_back(index + indexOffset); // Apply offset
					}
				}
			}
			else
			{
				// Generate sequential indices for the primitive
				size_t vertexCount = positions.size() / 3;
				for (size_t i = 0; i < vertexCount; ++i)
				{
					indices.push_back(static_cast<int>(i + indexOffset));
				}
			}

			// Update the index offset for the next primitive
			indexOffset += positions.size() / 3; // Increment by the number of vertices
		}

		// Interleave vertex data
		std::vector<float> interleavedVertexData;
		Geometry::GenerateInterleavedVertexData(positions, normals, texCoords, tangents, interleavedVertexData, key.attributesKey);

		// Create vertex and index buffers
		auto vertexBuffer = std::make_shared<VertexBuffer>(GL_ARRAY_BUFFER, GL_FLOAT, GL_STATIC_DRAW);
		vertexBuffer->Set(interleavedVertexData);

		auto indexBuffer = std::make_shared<IndexBuffer>(GL_ELEMENT_ARRAY_BUFFER, GL_UNSIGNED_INT, GL_STATIC_DRAW);
		indexBuffer->Set(indices);

		// Load material
		auto material = ParseMaterial(model, model.materials[key.materialIndex], key.materialIndex);

		// Create and return the batch
		return std::make_shared<Batch>(vertexBuffer, indexBuffer, material, false);
	}

	Material* GLBLoader::ParseMaterial(const tinygltf::Model& model, const tinygltf::Material& gltfMaterial, int matIdx)
	{
		// Check cache
		auto it = materialCache.find(matIdx);
		if (it != materialCache.end())
		{
			std::cout << "Using cached material for index: " << matIdx << std::endl;
			return it->second;
		}

		auto material = new Material(gltfMaterial.name.empty() ? "UnnamedMat" : gltfMaterial.name);
		int texId = 0;

		// Parse base color factor
		constexpr const char* BASE_COLOR_FACTOR = "baseColorFactor";
		if (gltfMaterial.values.find(BASE_COLOR_FACTOR) != gltfMaterial.values.end())
		{
			const auto& factor = gltfMaterial.values.at(BASE_COLOR_FACTOR).ColorFactor();
			material->baseColorFactor = glm::vec4(factor[0], factor[1], factor[2], factor[3]);
		}
		else
		{
			material->baseColorFactor = glm::vec4(1.0f);
		}

		// Parse base color texture
		constexpr const char* BASE_COLOR_TEXTURE = "baseColorTexture";
		if (gltfMaterial.values.find(BASE_COLOR_TEXTURE) != gltfMaterial.values.end())
		{
			int textureIndex = gltfMaterial.values.at(BASE_COLOR_TEXTURE).TextureIndex();
			auto texName = gltfMaterial.name + std::to_string(texId++);
			material->baseColorTexture = ParseTexture(model, texName, textureIndex);
		}

		// Parse metallic factor
		constexpr const char* METALLIC_FACTOR = "metallicFactor";
		if (gltfMaterial.values.find(METALLIC_FACTOR) != gltfMaterial.values.end())
		{
			const auto& param = gltfMaterial.values.at(METALLIC_FACTOR);
			if (param.has_number_value) // Check for valid numeric value
			{
				material->metallicFactor = static_cast<float>(param.Factor());
			}
		}
		else
		{
			material->metallicFactor = 1.0f;
		}

		// Parse roughness factor
		constexpr const char* ROUGHNESS_FACTOR = "roughnessFactor";
		if (gltfMaterial.values.find(ROUGHNESS_FACTOR) != gltfMaterial.values.end())
		{
			const auto& param = gltfMaterial.values.at(ROUGHNESS_FACTOR);
			if (param.has_number_value)
			{
				material->roughnessFactor = static_cast<float>(param.Factor());
			}
		}
		else
		{
			material->roughnessFactor = 1.0f;
		}

		// Parse metallic-roughness texture
		constexpr const char* METALLIC_ROUGHNESS_TEXTURE = "metallicRoughnessTexture";
		if (gltfMaterial.values.find(METALLIC_ROUGHNESS_TEXTURE) != gltfMaterial.values.end())
		{
			int textureIndex = gltfMaterial.values.at(METALLIC_ROUGHNESS_TEXTURE).TextureIndex();
			auto texName = gltfMaterial.name + std::to_string(texId++);
			material->metallicRoughnessTexture = ParseTexture(model, texName, textureIndex);
		}

		// Parse normal texture
		constexpr const char* NORMAL_TEXTURE = "normalTexture";
		if (gltfMaterial.additionalValues.find(NORMAL_TEXTURE) != gltfMaterial.additionalValues.end())
		{
			int textureIndex = gltfMaterial.additionalValues.at(NORMAL_TEXTURE).TextureIndex();
			auto texName = gltfMaterial.name + std::to_string(texId++);
			material->normalTexture = ParseTexture(model, texName, textureIndex);
		}

		// Parse occlusion texture
		constexpr const char* OCCLUSION_TEXTURE = "occlusionTexture";
		if (gltfMaterial.additionalValues.find(OCCLUSION_TEXTURE) != gltfMaterial.additionalValues.end())
		{
			int textureIndex = gltfMaterial.additionalValues.at(OCCLUSION_TEXTURE).TextureIndex();
			auto texName = gltfMaterial.name + std::to_string(texId++);
			material->occlusionTexture = ParseTexture(model, texName, textureIndex);
		}

		// Parse emissive texture
		constexpr const char* EMISSIVE_TEXTURE = "emissiveTexture";
		if (gltfMaterial.additionalValues.find(EMISSIVE_TEXTURE) != gltfMaterial.additionalValues.end())
		{
			int textureIndex = gltfMaterial.additionalValues.at(EMISSIVE_TEXTURE).TextureIndex();
			auto texName = gltfMaterial.name + std::to_string(texId++);
			material->emissiveTexture = ParseTexture(model, texName, textureIndex);
		}

		// Parse emissive factor
		constexpr const char* EMISSIVE_FACTOR = "emissiveFactor";
		if (gltfMaterial.additionalValues.find(EMISSIVE_FACTOR) != gltfMaterial.additionalValues.end())
		{
			const auto& factor = gltfMaterial.additionalValues.at(EMISSIVE_FACTOR).ColorFactor();
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

		// Cache the material
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

		auto jltexture = new Texture(name, width, height, data, channels);
		textureCache[textureIndex] = jltexture;

		return jltexture;
	}

	void GLBLoader::ParseTransform(std::shared_ptr<Node> node, const tinygltf::Node& gltfNode)
	{
		if (!gltfNode.matrix.empty())
		{
			glm::mat4 matrix(1.0f);
			std::memcpy(glm::value_ptr(matrix), gltfNode.matrix.data(), sizeof(float) * 16);
			node->localMatrix = matrix;
			return;
		}

		// Fallback to individual T/R/S components
		glm::vec3 translation(0.0f);
		glm::quat rotation(1.0f, 0.0f, 0.0f, 0.0f); // Identity quaternion
		glm::vec3 scale(1.0f);

		// Parse translation
		if (!gltfNode.translation.empty())
		{
			translation = glm::vec3(
				static_cast<float>(gltfNode.translation[0]),
				static_cast<float>(gltfNode.translation[1]),
				static_cast<float>(gltfNode.translation[2]));
		}

		// Parse rotation (GLTF quaternion format: x, y, z, w)
		if (!gltfNode.rotation.empty())
		{
			rotation = glm::quat(
				static_cast<float>(gltfNode.rotation[3]), // w
				static_cast<float>(gltfNode.rotation[0]), // x
				static_cast<float>(gltfNode.rotation[1]), // y
				static_cast<float>(gltfNode.rotation[2])); // z
		}

		// Parse scale
		if (!gltfNode.scale.empty())
		{
			scale = glm::vec3(
				static_cast<float>(gltfNode.scale[0]),
				static_cast<float>(gltfNode.scale[1]),
				static_cast<float>(gltfNode.scale[2]));
		}

		// Compose the local transformation matrix
		node->localMatrix = glm::translate(glm::mat4(1.0f), translation) *
			glm::mat4_cast(rotation) *
			glm::scale(glm::mat4(1.0f), scale);
	}

	std::string GLBLoader::GetAttributesKey(const std::map<std::string, int>& attributes)
	{
		std::string key;
		for (const auto& [name, accessorIndex] : attributes)
		{
			key += name + std::to_string(accessorIndex) + ";";
		}
		return key;
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

	void GLBLoader::LoadPositionAttribute(const tinygltf::Model& model, const tinygltf::Primitive& primitive,
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
	bool GLBLoader::LoadNormalAttribute(const tinygltf::Model& model, const tinygltf::Primitive& primitive,
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
	bool GLBLoader::LoadTexCoordAttribute(const tinygltf::Model& model, const tinygltf::Primitive& primitive,
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

	bool GLBLoader::LoadTangentAttribute(const tinygltf::Model& model, const tinygltf::Primitive& primitive, std::vector<float>& tangentData)
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
	bool GLBLoader::LoadIndices(const tinygltf::Model& model, const tinygltf::Primitive& primitive, std::vector<unsigned int>& indices)
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
			std::vector<unsigned int> indexVector;

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
			indices.insert(indices.end(), indexVector.begin(), indexVector.end());
			
			return true;
		}
		return false;
	}	
}