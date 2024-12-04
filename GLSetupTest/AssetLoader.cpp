#include "AssetLoader.h"
#include "GltfLoader.h"
#include "Geometry.h"
#include "Node.h"

#include <glm/glm.hpp>

namespace JLEngine
{
    AssetLoader::AssetLoader(ShaderManager* shaderManager, MeshManager* meshManager, MaterialManager* materialManager, TextureManager* textureManager)
        : m_shaderManager(shaderManager), m_meshManager(meshManager), m_materialManager(materialManager), m_textureManager(textureManager)
    {

    }
    std::unique_ptr<Node> AssetLoader::LoadGLB(const std::string& glbFile)
    {
		tinygltf::TinyGLTF loader;
		tinygltf::Model model;
		std::string err;
		std::string warn;

		bool ret = loader.LoadBinaryFromFile(&model, &err, &warn, glbFile);
		if (!ret)
		{
			std::cout << err << std::endl;
			return nullptr;
		}

		auto meshes = loadMeshes(model);
		auto materials = loadMaterials(model);

        // Map to store GLTF node indices to scene nodes
        std::unordered_map<int, std::unique_ptr<Node>> nodeMap;

        // Create all nodes first
        for (size_t i = 0; i < model.nodes.size(); ++i)
        {
            const auto& gltfNode = model.nodes[i];
            auto node = std::make_unique<Node>(gltfNode.name);

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

            // Set transformation properties
            if (!gltfNode.translation.empty())
            {
                node->translation = glm::vec3(
                    gltfNode.translation[0],
                    gltfNode.translation[1],
                    gltfNode.translation[2]);
            }
            if (!gltfNode.rotation.empty())
            {
                node->rotation = glm::quat(
                    static_cast<float>(gltfNode.rotation[3]), // w
                    static_cast<float>(gltfNode.rotation[0]), // x
                    static_cast<float>(gltfNode.rotation[1]), // y
                    static_cast<float>(gltfNode.rotation[2])); // 
            }
            if (!gltfNode.scale.empty())
            {
                node->scale = glm::vec3(
                    gltfNode.scale[0],
                    gltfNode.scale[1],
                    gltfNode.scale[2]);
            }
            if (!gltfNode.matrix.empty())
            {
                node->useMatrix = true;
                node->localMatrix = glm::mat4(
                    gltfNode.matrix[0], gltfNode.matrix[1], gltfNode.matrix[2], gltfNode.matrix[3],
                    gltfNode.matrix[4], gltfNode.matrix[5], gltfNode.matrix[6], gltfNode.matrix[7],
                    gltfNode.matrix[8], gltfNode.matrix[9], gltfNode.matrix[10], gltfNode.matrix[11],
                    gltfNode.matrix[12], gltfNode.matrix[13], gltfNode.matrix[14], gltfNode.matrix[15]
                );
                node->translation = glm::vec3(gltfNode.matrix[12], gltfNode.matrix[13], gltfNode.matrix[14]);
                node->scale = glm::vec3(
                    glm::length(glm::vec3(gltfNode.matrix[0], gltfNode.matrix[1], gltfNode.matrix[2])),   
                    glm::length(glm::vec3(gltfNode.matrix[4], gltfNode.matrix[5], gltfNode.matrix[6])),  
                    glm::length(glm::vec3(gltfNode.matrix[8], gltfNode.matrix[9], gltfNode.matrix[10]))   
                );
                glm::mat3 rotationMatrix = {
                    glm::vec3(gltfNode.matrix[0], gltfNode.matrix[1], gltfNode.matrix[2]) / node->scale.x,
                    glm::vec3(gltfNode.matrix[4], gltfNode.matrix[5], gltfNode.matrix[6]) / node->scale.y,
                    glm::vec3(gltfNode.matrix[8], gltfNode.matrix[9], gltfNode.matrix[10]) / node->scale.z
                };
                node->rotation = glm::quat_cast(rotationMatrix);
            }
            else
            {
                node->useMatrix = false;
                node->translation = glm::vec3(0.0f);
                node->scale = glm::vec3(1.0f);
                node->rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
            }
            
            // Attach meshes
            if (gltfNode.mesh >= 0 && gltfNode.mesh < meshes.size())
            {
                Mesh* mesh = meshes[gltfNode.mesh];
                node->meshes.push_back(mesh);

                // Attach materials to the mesh
                const auto& gltfMesh = model.meshes[gltfNode.mesh];
                for (size_t primitiveIdx = 0; primitiveIdx < gltfMesh.primitives.size(); ++primitiveIdx)
                {
                    int materialIdx = gltfMesh.primitives[primitiveIdx].material;
                    if (materialIdx >= 0 && materialIdx < materials.size())
                    {
                        mesh->AddMaterial(materials[materialIdx]);
                    }
                    else
                    {
                        // Handle cases where no material is specified
                        mesh->AddMaterial(m_materialManager->GetDefaultMaterial());
                    }
                }

                // Ensure number of index buffers matches number of materials
                if (mesh->GetIndexBuffers().size() != mesh->GetMaterials().size())
                {
                    std::cerr << "Error: Mismatch between index buffers and materials in mesh: " << mesh->GetName() << std::endl;
                }
            }

            nodeMap[(int)i] = std::move(node);
        }

        // Establish parent-child relationships
        for (size_t i = 0; i < model.nodes.size(); ++i)
        {
            const auto& gltfNode = model.nodes[i];
            auto& parentNode = nodeMap[(int)i];

            for (int childIndex : gltfNode.children)
            {
                auto& childNode = nodeMap[childIndex];
                parentNode->AddChild(std::move(childNode));
            }
        }

        //PrintNodeHierarchy(rootNode.get());

        return std::move(nodeMap[0]); // Return raw pointer as per your current implementation
    }

	std::vector<Mesh*> AssetLoader::loadMeshes(tinygltf::Model& model)
	{
		auto meshes = std::vector<Mesh*>();

		for (const auto& mesh : model.meshes)
		{
			if (mesh.primitives.size() == 1)
			{
                auto name = mesh.name + "_Primitive0";
				auto jlmesh = PrimitiveFromMesh(model, name, mesh.primitives[0], m_meshManager);
				meshes.push_back(jlmesh);
			}
			else if (mesh.primitives.size() > 1)
			{
                std::string name = mesh.name;
				auto jlmesh = MergePrimitivesToMesh(model, name, mesh, m_meshManager);
				meshes.push_back(jlmesh);
			}
		}
		return meshes;
	}

	std::vector<Material*> AssetLoader::loadMaterials(tinygltf::Model& model)
	{
		auto materials = std::vector<Material*>();
		for (const auto& mat : model.materials)
		{
			auto jlmat = LoadMaterial(model, mat, m_materialManager, m_textureManager);
			materials.push_back(jlmat);
		}
		return materials;
	}
}