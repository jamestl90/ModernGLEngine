#include "GLBLoader.h"

#include "Material.h"
#include "Mesh.h"
#include "Node.h"
#include "Texture.h"
#include "GraphicsAPI.h"
#include "ResourceLoader.h"
#include "JLHelpers.h"

#include <tiny_gltf.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include "Geometry.h"
#include <glm/gtx/string_cast.hpp>

namespace JLEngine
{
    GLBLoader::GLBLoader(ResourceLoader* resourceLoader) 
		: m_resourceLoader(resourceLoader)
    {
    }

	std::shared_ptr<Node> GLBLoader::LoadGLB(const std::string& fileName)
	{
		std::cout << "Loading GLB: " << fileName << std::endl;

		// Load GLB file using tinygltf
		tinygltf::TinyGLTF loader;
		tinygltf::Model model;
		std::string err, warn;

		bool ret = loader.LoadBinaryFromFile(&model, &err, &warn, fileName);

		if (!warn.empty())
		{
			std::cerr << "GLBLoader Warning: " << warn << std::endl;
		}

		if (!ret)
		{
			std::cerr << "GLBLoader Error: Failed to load GLB file: " << err << std::endl;
			return nullptr;
		}

		if (model.scenes.empty())
		{
			std::cerr << "GLBLoader Error: No scenes found in GLB file." << std::endl;
			return nullptr;
		}

		// Retrieve the default scene or the first scene
		const tinygltf::Scene& scene = model.scenes[model.defaultScene >= 0 ? model.defaultScene : 0];
		auto rootNode = std::make_shared<Node>("RootNode");
		rootNode->SetTag(NodeTag::Default);

		// Parse all nodes in the scene
		for (int nodeIndex : scene.nodes)
		{
			if (nodeIndex < 0 || nodeIndex >= model.nodes.size())
			{
				std::cerr << "GLBLoader Warning: Invalid node index in scene." << std::endl;
				continue;
			}

			const tinygltf::Node& gltfNode = model.nodes[nodeIndex];
			auto childNode = ParseNode(model, gltfNode);
			childNode->name = gltfNode.name;

			if (scene.nodes.size() == 1)
				return childNode;

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

		// Handle mesh
		if (gltfNode.mesh >= 0)
		{
			node->SetTag(NodeTag::Mesh);

			// Track which nodes reference this mesh
			auto& referencingNodes = meshNodeReferences[gltfNode.mesh];
			referencingNodes.push_back(node);

			// Parse the mesh
			node->mesh = ParseMesh(model, gltfNode.mesh);

			// Debug: Log the mesh association
			//std::cout << "Node " << node->name << " references mesh " << node->mesh->GetName() << std::endl;
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

		// Parse and apply transformations
		ParseTransform(node, gltfNode);

		int stopTest = 5;
		int stopIndex = 0;
		// Recursively parse child nodes
		for (int childIndex : gltfNode.children)
		{

			if (childIndex < 0 || childIndex >= model.nodes.size())
			{
				std::cerr << "Warning: Invalid child index in node " << node->name << std::endl;
				continue;
			}

			const tinygltf::Node& childGltfNode = model.nodes[childIndex];
			auto childNode = ParseNode(model, childGltfNode);
			if (childNode)
			{
				node->AddChild(childNode);
			}
			stopIndex++;
		}

		return node;
	}

	std::shared_ptr<Mesh> GLBLoader::ParseMesh(const tinygltf::Model& model, int meshIndex)
	{
		// Check if the mesh is already cached
		auto it = meshCache.find(meshIndex);
		if (it != meshCache.end())
		{
			return it->second; // Return cached mesh
		}

		if (meshIndex < 0 || meshIndex >= model.meshes.size())
		{
			std::cerr << "GLBLoader Error: Invalid mesh index " << meshIndex << "." << std::endl;
			return nullptr;
		}

		const tinygltf::Mesh& gltfMesh = model.meshes[meshIndex];

		// Create a new Mesh object
		auto mesh = m_resourceLoader->CreateMesh(gltfMesh.name.empty() ? "UnnamedMesh" : gltfMesh.name);
		mesh->SetStatic(true);

		std::cout << "Mesh name: " << model.meshes[meshIndex].name << std::endl;

		// Group primitives by material and attributes
		std::unordered_map<MaterialVertexAttributeKey, std::vector<const tinygltf::Primitive*>> groups;
		for (const auto& primitive : gltfMesh.primitives)
		{
			auto vertexAttribKey = GenerateVertexAttribKey(primitive.attributes);
			if (vertexAttribKey == 7 || vertexAttribKey == 39)
			{
				// auto generate tangents for POS/NORMAL/UV
				AddToVertexAttribKey(vertexAttribKey, AttributeType::TANGENT);
			}
			if (vertexAttribKey == 1)
			{
				AddToVertexAttribKey(vertexAttribKey, AttributeType::NORMAL);
			}
			if (HasVertexAttribKey(vertexAttribKey, AttributeType::TEX_COORD_1))
			{
				RemoveFromVertexAttribKey(vertexAttribKey, AttributeType::TEX_COORD_1);
			}
			MaterialVertexAttributeKey key(primitive.material, vertexAttribKey);
			groups[key].push_back(&primitive);
		}
		
		// Create batches for each grouped set of primitives
		for (const auto& [key, primitives] : groups)
		{
			auto subMesh = CreateSubMesh(model, primitives, key);

			mesh->AddSubmesh(subMesh);
		}

		// Cache the mesh for future use
		meshCache[meshIndex] = mesh;

		return mesh;
	}

	SubMesh GLBLoader::CreateSubMesh(const tinygltf::Model& model, 
		const std::vector<const tinygltf::Primitive*>& primitives, MaterialVertexAttributeKey key)
	{		
		std::vector<float> positions, normals, texCoords, tangents, texCoords2;
		std::vector<uint32_t> indices;

		uint32_t indexOffset = 0;
		BatchLoadAttributes(model, primitives, positions, normals, texCoords, texCoords2, tangents, indices, indexOffset, key.attributesKey);

		GenerateMissingAttributes(positions, normals, texCoords, tangents, indices, key.attributesKey);

		// Interleave vertex data
		std::vector<float> interleavedVertexData;
		Geometry::GenerateInterleavedVertexData(positions, normals, texCoords, texCoords2, tangents, interleavedVertexData);

		Material* material;
		if (!model.materials.empty())
		{
			material = ParseMaterial(model, model.materials[key.materialIndex], key.materialIndex).get();
		}
		else
		{
			material = m_resourceLoader->GetDefaultMaterial();
		}

		auto& vao = m_staticVAOs[key.attributesKey];
		if (!vao)
		{
			//std::cerr << "Error: No VAO found for attributes key " << key.attributesKey << "\n";
			
			vao = m_resourceLoader->CreateVertexArray("Vao: " + key.attributesKey);
			m_staticVAOs[key.attributesKey] = vao;			
			vao->SetVertexAttribKey(key.attributesKey);
		}

		auto& vbo = vao->GetVBO();
		//if (vbo.Size() + interleavedVertexData.size() * sizeof(float) > vbo.GetBuffer().capacity()) 
		//{
		//	std::cerr << "Error: Vertex buffer capacity exceeded.\n";
		//	return {};
		//}
		uint32_t vertexOffset = (uint32_t)vbo.Size() / (CalculateStride(key.attributesKey) / 4);
		vbo.Append(interleavedVertexData);

		auto& ibo = vao->GetIBO();
		//if (ibo.Size() + indices.size() * sizeof(uint32_t) > ibo.GetBuffer().capacity()) {
		//	std::cerr << "Error: Index buffer capacity exceeded.\n";
		//	return {};
		//}
		uint32_t indexBase = (uint32_t)ibo.Size();
		ibo.Append(indices);

		SubMesh submesh;
		submesh.attribKey = key.attributesKey;
		submesh.materialHandle = material->GetHandle();
		submesh.command = {
			.count = static_cast<uint32_t>(indices.size()),
			.instanceCount = 1,
			.firstIndex = indexBase,
			.baseVertex = vertexOffset,
			.baseInstance = 0
		};

		return submesh;
	}

	std::shared_ptr<Material> GLBLoader::ParseMaterial(const tinygltf::Model& model, const tinygltf::Material& gltfMaterial, int matIdx)
	{
		// Check cache
		auto it = materialCache.find(matIdx);
		if (it != materialCache.end())
		{
			//std::cout << "Using cached material for index: " << matIdx << std::endl;
			return it->second;
		}

		auto material = m_resourceLoader->CreateMaterial(gltfMaterial.name.empty() ? "UnnamedMat" : gltfMaterial.name);
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
		material->alphaMode = AlphaModeFromString(gltfMaterial.alphaMode);
		material->alphaCutoff = static_cast<float>(gltfMaterial.alphaCutoff);
		material->doubleSided = gltfMaterial.doubleSided;

		// Cache the material
		materialCache[matIdx] = material;

		return material;
	}

	std::shared_ptr<Texture> GLBLoader::ParseTexture(const tinygltf::Model& model, std::string& name, int textureIndex)
	{
		// Check if the texture index is valid
		if (textureIndex < 0 || textureIndex >= model.textures.size())
			return nullptr;

		// Check the cache for an existing texture
		auto it = textureCache.find(textureIndex);
		if (it != textureCache.end())
		{
			return it->second; // Return the cached texture
		}

		// Fetch the texture and its image data
		const auto& texture = model.textures[textureIndex];
		const auto& glbImageData = model.images[texture.source];

		// Generate a final name for the texture
		const std::string& finalName = name + (texture.name.empty() ? "UnnamedTexture" : texture.name);

		// Extract texture details
		uint32_t width = static_cast<uint32_t>(glbImageData.width);
		uint32_t height = static_cast<uint32_t>(glbImageData.height);
		int channels = glbImageData.component;
		ImageData imgData;
		imgData.width = width;
		imgData.height = height;
		imgData.channels = channels;
		imgData.data = glbImageData.image;

		// Create a new texture
		auto jltexture = m_resourceLoader->CreateTexture(finalName, imgData);

		// Cache the newly created texture
		textureCache[textureIndex] = jltexture;

		return jltexture;
	}

	void GLBLoader::ParseTransform(std::shared_ptr<Node> node, const tinygltf::Node& gltfNode)
	{
		if (!gltfNode.matrix.empty() && gltfNode.matrix.size() == 16)
		{
			// Load the matrix directly
			glm::mat4 matrix(1.0f);
			for (int i = 0; i < 16; ++i) {
				matrix[i / 4][i % 4] = static_cast<float>(gltfNode.matrix[i]);
			}
			node->localMatrix = matrix;

			// Decompose the matrix into T/R/S
			glm::vec3 skew;
			glm::vec4 perspective;
			if (!glm::decompose(matrix, node->scale, node->rotation, node->translation, skew, perspective))
			{
				std::cerr << "Error: Failed to decompose matrix in GLTF node." << std::endl;
				node->translation = glm::vec3(0.0f);
				node->rotation = glm::quat_identity<float, glm::defaultp>();
				node->scale = glm::vec3(1.0f);
			}
			return;
		}

		// Load T/R/S components and compose the matrix
		node->translation = glm::vec3(0.0f);
		node->rotation = glm::quat_identity<float, glm::defaultp>();
		node->scale = glm::vec3(1.0f);

		// Parse translation
		if (!gltfNode.translation.empty() && gltfNode.translation.size() == 3)
		{
			node->translation = glm::vec3(
				gltfNode.translation[0],
				gltfNode.translation[1],
				gltfNode.translation[2]);
		}

		// Parse rotation (GLTF quaternion format: x, y, z, w)
		if (!gltfNode.rotation.empty() && gltfNode.rotation.size() == 4)
		{
			node->rotation = glm::quat(
				static_cast<float>(gltfNode.rotation[3]), // w
				static_cast<float>(gltfNode.rotation[0]), // x
				static_cast<float>(gltfNode.rotation[1]), // y
				static_cast<float>(gltfNode.rotation[2])); // z
		}

		// Parse scale
		if (!gltfNode.scale.empty() && gltfNode.scale.size() == 3)
		{
			node->scale = glm::vec3(
				gltfNode.scale[0],
				gltfNode.scale[1],
				gltfNode.scale[2]);
		}

		// Compose the matrix from T/R/S
		node->localMatrix = glm::translate(glm::mat4(1.0f), node->translation) *
			glm::mat4_cast(node->rotation) *
			glm::scale(glm::mat4(1.0f), node->scale);
	}

	void GLBLoader::BatchLoadAttributes(const tinygltf::Model& model,
		const std::vector<const tinygltf::Primitive*>& primitives,
		std::vector<float>& positions,
		std::vector<float>& normals,
		std::vector<float>& texCoords,
		std::vector<float>& texCoords2,
		std::vector<float>& tangents,
		std::vector<uint32_t>& indices,
		uint32_t& indexOffset, 
		VertexAttribKey key)
	{
		for (const auto* primitive : primitives)
		{
			uint32_t currentVertexCount = static_cast<uint32_t>(positions.size() / 3); // Before adding new vertices

			// Load vertex attributes
			LoadPositionAttribute(model, *primitive, positions);
			if (HasVertexAttribKey(key, AttributeType::NORMAL))
				LoadNormalAttribute(model, *primitive, normals);
			if (HasVertexAttribKey(key, AttributeType::TEX_COORD_0))
				LoadTexCoordAttribute(model, *primitive, texCoords);
			if (HasVertexAttribKey(key, AttributeType::TANGENT))
				LoadTangentAttribute(model, *primitive, tangents);
			if (HasVertexAttribKey(key, AttributeType::TEX_COORD_1))
				LoadTexCoord2Attribute(model, *primitive, texCoords2);

			// Load indices or generate sequential ones
			if (primitive->indices >= 0)
			{
				std::vector<unsigned int> tempIndexBuffer;
				if (LoadIndices(model, *primitive, tempIndexBuffer))
				{
					for (auto index : tempIndexBuffer)
					{
						indices.push_back(index + currentVertexCount); // Apply current offset
					}
				}
			}
			else
			{
				size_t vertexCount = positions.size() / 3 - currentVertexCount;
				for (size_t i = 0; i < vertexCount; ++i)
				{
					indices.push_back(static_cast<int>(i + currentVertexCount));
				}
			}
		}
	}

	void GLBLoader::LoadAttributes(const tinygltf::Model& model,
		const tinygltf::Primitive* primitive,
		std::vector<float>& positions,
		std::vector<float>& normals,
		std::vector<float>& texCoords,
		std::vector<float>& tangents,
		std::vector<uint32_t>& indices,
		uint32_t& indexOffset)
	{

		uint32_t currentVertexCount = static_cast<uint32_t>(positions.size() / 3); // Before adding new vertices

		// Load vertex attributes
		LoadPositionAttribute(model, *primitive, positions);
		LoadNormalAttribute(model, *primitive, normals);
		LoadTexCoordAttribute(model, *primitive, texCoords);
		LoadTangentAttribute(model, *primitive, tangents);

		// Load indices or generate sequential ones
		if (primitive->indices >= 0)
		{
			std::vector<unsigned int> tempIndexBuffer;
			if (LoadIndices(model, *primitive, tempIndexBuffer))
			{
				for (auto index : tempIndexBuffer)
				{
					indices.push_back(index + currentVertexCount); // Apply current offset
				}
			}
		}
		else
		{
			size_t vertexCount = positions.size() / 3 - currentVertexCount;
			for (size_t i = 0; i < vertexCount; ++i)
			{
				indices.push_back(static_cast<int>(i + currentVertexCount));
			}
		}
	}

	void GLBLoader::GenerateMissingAttributes(std::vector<float>& positions,
		std::vector<float>& normals,
		const std::vector<float>& texCoords,
		std::vector<float>& tangents, 
		const std::vector<uint32_t>& indices,
		VertexAttribKey key)
	{
		// Generate normals if only positions are present
		if (normals.empty() && texCoords.empty())
		{
			if (HasVertexAttribKey(key, AttributeType::NORMAL))
			{
				std::cout << "Generating normals..." << std::endl;
				normals = Geometry::CalculateFlatNormals(positions);
			}
		}

		// Generate tangents if UVs and normals are available but tangents are missing
		if (tangents.empty() && !texCoords.empty() && !normals.empty())
		{
			if (HasVertexAttribKey(key, AttributeType::TANGENT))
			{
				//std::cout << "Generating tangents..." << std::endl;
				tangents = Geometry::CalculateTangents(positions, normals, texCoords, indices);
			}
		}
		else if (tangents.empty() && (!texCoords.empty() || !normals.empty()))
		{
			std::cerr << "Warning: Cannot generate tangents without both UVs and normals." << std::endl;
		}
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

	bool GLBLoader::LoadTexCoord2Attribute(const tinygltf::Model& model, const tinygltf::Primitive& primitive,
		std::vector<float>& vertexData)
	{
		const auto& texCoordAttr = primitive.attributes.find("TEXCOORD_1");
		if (texCoordAttr != primitive.attributes.end())
		{
			const tinygltf::Accessor& texCoordAccessor = model.accessors[texCoordAttr->second];
			const tinygltf::BufferView& texCoordBufferView = model.bufferViews[texCoordAccessor.bufferView];
			const tinygltf::Buffer& texCoordBuffer = model.buffers[texCoordBufferView.buffer];

			// Validate component type
			if (texCoordAccessor.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT) {
				std::cerr << "Error: Unsupported TEXCOORD_1 component type: " << texCoordAccessor.componentType << std::endl;
				return false;
			}

			size_t stride = texCoordAccessor.ByteStride(model.bufferViews[texCoordAccessor.bufferView]);
			if (stride != sizeof(float) * 2)
			{
				std::cerr << "Error: Unsupported TEXCOORD_1 stride: " << stride << std::endl;
			}

			// Validate buffer bounds
			size_t requiredSize = texCoordBufferView.byteOffset + texCoordAccessor.byteOffset +
				texCoordAccessor.count * 2 * sizeof(float);
			if (requiredSize > texCoordBuffer.data.size()) {
				std::cerr << "Error: TEXCOORD_1 accessor exceeds buffer bounds!" << std::endl;
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
			std::cerr << "Warning: TEXCOORD_1 attribute not found in primitive!" << std::endl;
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
			//std::cerr << "Warning: TANGENT attribute not found in primitive" << std::endl;
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

	void GLBLoader::ClearCaches()
	{
		meshCache.clear();
		materialCache.clear();
		textureCache.clear();
		meshNodeReferences.clear();
	}
}