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

	JLEngine::Mesh* PrimitiveFromMesh(const tinygltf::Model& model, const tinygltf::Primitive& primitive, MeshManager* meshMgr)
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
		auto jlmesh = meshMgr->LoadMeshFromData("Primitive", vbo, ibo);
		return jlmesh;
	}

	JLEngine::Mesh* MergePrimitivesToMesh(const tinygltf::Model& model, const tinygltf::Mesh& mesh, MeshManager* meshMgr)
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
		auto jlmesh = meshMgr->CreateEmptyMesh("MeshWithMultiplePrimitives");
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

	JLEngine::Material* LoadMaterial(const tinygltf::Model& model, const tinygltf::Material& gltfMaterial, MaterialManager* matMgr)
	{
		auto material = matMgr->CreateMaterial("matName");

		return material;
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
