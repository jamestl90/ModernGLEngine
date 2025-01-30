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

		for (auto i = 0; i < model.animations.size(); i++)
		{
			const auto& gltfAnim = model.animations[i];
			ParseAnimation(i, model, gltfAnim);
		}

		// Parse all nodes in the scene
		for (int nodeIndex : scene.nodes)
		{
			if (nodeIndex < 0 || nodeIndex >= model.nodes.size())
			{
				std::cerr << "GLBLoader Warning: Invalid node index in scene." << std::endl;
				continue;
			}

			const tinygltf::Node& gltfNode = model.nodes[nodeIndex];
			auto childNode = ParseNode(model, gltfNode, nodeIndex); 
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

	std::shared_ptr<Node> GLBLoader::ParseNode(const tinygltf::Model& model, const tinygltf::Node& gltfNode, int nodeIndex)
	{
		auto node = std::make_shared<Node>(gltfNode.name.empty() ? "UnnamedNode" : gltfNode.name);
		m_nodeList.push_back(node.get());

		// Parse and apply transformations
		ParseTransform(node, gltfNode);

		// Handle mesh
		if (gltfNode.mesh >= 0)
		{
			node->SetTag(NodeTag::Mesh);

			auto& meshName = model.meshes[gltfNode.mesh].name;
			auto existingMesh = m_resourceLoader->Get<Mesh>(meshName);

			// Track which nodes reference this mesh
			//auto& referencingNodes = meshNodeReferences[gltfNode.mesh];

			// Check if it's an instance (i.e., if the mesh has been referenced before)
			if (existingMesh != nullptr)
			{
				auto& masterNode = existingMesh->node;  

				for (size_t submeshIndex = 0; submeshIndex < existingMesh->GetSubmeshes().size(); ++submeshIndex)
				{
					auto& submesh = existingMesh->GetSubmesh((int)submeshIndex);
					if (submesh.instanceTransforms == nullptr)
					{
						// add the first one
						submesh.command.instanceCount = 1;
						submesh.instanceTransforms = std::make_shared<std::vector<Node*>>();
						submesh.instanceTransforms->push_back(masterNode);
					}
					if (gltfNode.skin >= 0)
					{
						node->animController = std::make_shared<AnimationController>();
						node->animController->SetSkeleton(existingMesh->GetSkeleton());
					}

					submesh.command.instanceCount++;
					submesh.instanceTransforms->push_back(node.get());
				}
				node->mesh = existingMesh;
			}
			else
			{
				node->mesh = ParseMesh(model, gltfNode.mesh);
				node->mesh->node = node.get(); // set the node to the "owner" node

				if (gltfNode.skin >= 0)
				{
					const auto& skin = model.skins[gltfNode.skin];
					ParseSkin(model, skin, *node->mesh);
					
					if (node->animController == nullptr)
					{
						node->animController = std::make_shared<AnimationController>();
						node->animController->SetSkeleton(node->mesh->GetSkeleton());
					}
				}
			}
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

		// Recursively parse child nodes
		for (int childIndex : gltfNode.children)
		{
			if (childIndex < 0 || childIndex >= model.nodes.size())
			{
				std::cerr << "Warning: Invalid child index in node " << node->name << std::endl;
				continue;
			}

			const tinygltf::Node& childGltfNode = model.nodes[childIndex];
			auto childNode = ParseNode(model, childGltfNode, childIndex);
			if (childNode)
			{
				node->AddChild(childNode);
			}
		}

		return node;
	}

	std::shared_ptr<Mesh> GLBLoader::ParseMesh(const tinygltf::Model& model, int meshIndex)
	{
		// Check if the mesh is already cached
		auto it = meshCache.find(meshIndex);
		if (it != meshCache.end())
		{
			return it->second; 
		}

		if (meshIndex < 0 || meshIndex >= model.meshes.size())
		{
			std::cerr << "GLBLoader Error: Invalid mesh index " << meshIndex << "." << std::endl;
			return nullptr;
		}

		const tinygltf::Mesh& gltfMesh = model.meshes[meshIndex];

		// Create a new Mesh object
		auto mesh = m_resourceLoader->CreateMesh(gltfMesh.name.empty() ? "UnnamedMesh" : gltfMesh.name);

		std::cout << "Mesh name: " << model.meshes[meshIndex].name << std::endl;

		if (mesh->GetName() == "UnnamedMesh")
		{
			std::cout << "Warning: Mesh doesn't have a name. Could lead to duplicates" << std::endl;
		}

		// Group primitives by material and attributes
		std::unordered_map<MaterialVertexAttributeKey, std::vector<const tinygltf::Primitive*>> groups;
		for (const auto& primitive : gltfMesh.primitives)
		{
			auto vertexAttribKey = GenerateVertexAttribKey(primitive.attributes);
			if (vertexAttribKey == 7 || vertexAttribKey == 39 || vertexAttribKey == 199)
			{
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
		
		for (const auto& [key, primitives] : groups)
		{
			// if its got joint attrib we can assume it has skinning vertex data
			bool skinnedMesh = HasVertexAttribKey(key.attributesKey, AttributeType::JOINT_0);
			SubMesh submesh;
			if (skinnedMesh)
			{
				submesh = CreateSubMeshAnim(model, primitives, key);
			}
			else
			{
				submesh = CreateSubMesh(model, primitives, key);
			}
			mesh->AddSubmesh(submesh);
		}

		meshCache[meshIndex] = mesh;

		return mesh;
	}

	std::shared_ptr<Animation> GLBLoader::ParseAnimation(int animIdx, const tinygltf::Model& model, const tinygltf::Animation& gltfAnimation)
	{
		std::string animName;
		std::string nodeName;
		int targetNode = gltfAnimation.channels[0].target_node;
		if (targetNode >= 0 && targetNode < model.nodes.size())
		{
			nodeName = model.nodes[targetNode].name.empty() ? "UnnamedNode" : model.nodes[targetNode].name;
		}

		if (animName.empty())
			animName = "Anim_" + nodeName + "_idx:" + std::to_string(animIdx);
		else
			animName = gltfAnimation.name + "_" + nodeName + "_idx:" + std::to_string(animIdx);

		auto cachedAnim = m_resourceLoader->Get<Animation>(animName);
		if (cachedAnim != nullptr)
		{
			return cachedAnim;
		}

		auto animation = m_resourceLoader->CreateAnimation(animName);

		// Parse samplers
		for (const auto& sampler : gltfAnimation.samplers)
		{
			AnimationSampler animSampler;

			auto inputTimes = GetKeyframeTimes(model, sampler.input);
			auto outputValues = GetKeyframeValues(model, sampler.output);

			if (inputTimes.size() != outputValues.size())
			{
				std::cerr << "Sampler input/output size mismatch\n";
			}

			animSampler.SetTimes(std::move(inputTimes));
			animSampler.SetValues(std::move(outputValues));
			animSampler.SetInterpolation(sampler.interpolation.empty() ? 
				InterpolationType::LINEAR : InterpolationFromString(sampler.interpolation));

			animation->AddSampler(animSampler);
		}

		// Parse channels
		for (const auto& channel : gltfAnimation.channels)
		{
			int targetNode = channel.target_node;
			if (targetNode < 0 || targetNode >= model.nodes.size())
			{
				targetNode = -1; // Or skip the channel if it doesn't target a node
			}
			AnimationChannel animChannel(channel.sampler, targetNode, TargetPathFromString(channel.target_path));
			animation->AddChannel(animChannel);
		}

		animation->CalcDuration();

		return animation;
	}

	void GLBLoader::ParseSkin(const tinygltf::Model& model, const tinygltf::Skin& skin, Mesh& mesh)
	{
		auto& inverseBindMatrices = mesh.GetInverseBindMatrices();

		// Load inverse bind matrices
		if (skin.inverseBindMatrices >= 0)
		{
			const auto& accessor = model.accessors[skin.inverseBindMatrices];
			const auto& bufferView = model.bufferViews[accessor.bufferView];
			const auto& buffer = model.buffers[bufferView.buffer];

			const glm::mat4* data = reinterpret_cast<const glm::mat4*>(
				&buffer.data[bufferView.byteOffset + accessor.byteOffset]);
			size_t count = accessor.count;

			// Ensure the count matches the number of joints
			assert(count == skin.joints.size());

			inverseBindMatrices.resize(count);
			std::copy(data, data + count, inverseBindMatrices.begin());
		}

		// Create skeleton and initialize joints
		Skeleton skeleton;
		skeleton.name = skin.name;
		skeleton.joints.resize(skin.joints.size());

		// Precompute parent-child relationships
		std::unordered_map<int, int> nodeToParent;
		for (size_t i = 0; i < model.nodes.size(); ++i)
		{
			const auto& node = model.nodes[i];
			for (int child : node.children)
			{
				nodeToParent[child] = static_cast<int>(i);
			}
		}

		// Populate skeleton joints
		for (size_t i = 0; i < skin.joints.size(); ++i)
		{
			int jointIndex = skin.joints[i];
			const auto& jointNode = model.nodes[jointIndex];

			Skeleton::Joint joint;

			// Resolve parent index
			auto it = nodeToParent.find(jointIndex);
			if (it != nodeToParent.end())
			{
				// Map parent GLTF node index to joint index
				auto parentIt = std::find(skin.joints.begin(), skin.joints.end(), it->second);
				joint.parentIndex = (parentIt != skin.joints.end())
					? static_cast<int>(std::distance(skin.joints.begin(), parentIt))
					: -1;
			}
			else
			{
				joint.parentIndex = -1; // No parent (root node)
			}

			// Compute local transform
			glm::vec3 translation = jointNode.translation.empty()
				? glm::vec3(0.0f)
				: glm::vec3(glm::make_vec3(jointNode.translation.data())); 
			glm::quat rotation = jointNode.rotation.empty()
				? glm::quat_identity<float, glm::defaultp>()
				: glm::quat(static_cast<float>(jointNode.rotation[3]), // w
					static_cast<float>(jointNode.rotation[0]), // x
					static_cast<float>(jointNode.rotation[1]), // y
					static_cast<float>(jointNode.rotation[2])); // z
			glm::vec3 scale = jointNode.scale.empty()
				? glm::vec3(1.0f)
				: glm::vec3(glm::make_vec3(jointNode.scale.data()));
			glm::mat4 localTransform = glm::translate(glm::mat4(1.0f), translation) *
				glm::mat4_cast(rotation) *
				glm::scale(glm::mat4(1.0f), scale);

			joint.localTransform = localTransform;
			skeleton.joints[i] = joint;
		}

		// Map GLTF node indices to joint indices for animations
		std::unordered_map<int, int> gltfNodeToJointIndex;
		for (size_t i = 0; i < skin.joints.size(); ++i)
		{
			gltfNodeToJointIndex[skin.joints[i]] = static_cast<int>(i);
		}

		// Update animation target nodes
		auto animations = GetAnimationsFromSkeleton(skin.skeleton);
		for (auto anim : animations)
		{
			for (auto& channel : anim->GetChannels())
			{
				if (channel.IsUpdated()) continue;

				int gltfNodeIndex = channel.GetTargetNode();
				auto it = gltfNodeToJointIndex.find(gltfNodeIndex);
				if (it != gltfNodeToJointIndex.end())
				{
					channel.UpdateTargetNode(it->second);
				}
				else
				{
					channel.UpdateTargetNode(-1); // Invalid joint index
				}
			}
		}

		// Assign the skeleton to the mesh
		mesh.SetSkeleton(std::make_shared<Skeleton>(skeleton));
	}

	std::vector<float> GLBLoader::GetKeyframeTimes(const tinygltf::Model& model, int accessorIndex)
	{
		const auto& accessor = model.accessors[accessorIndex];
		const auto& bufferView = model.bufferViews[accessor.bufferView];
		const auto& buffer = model.buffers[bufferView.buffer];

		const float* data = reinterpret_cast<const float*>(&buffer.data[bufferView.byteOffset + accessor.byteOffset]);
		return std::vector<float>(data, data + accessor.count);
	}

	std::vector<glm::vec4> GLBLoader::GetKeyframeValues(const tinygltf::Model& model, int accessorIndex)
	{
		const auto& accessor = model.accessors[accessorIndex];
		const auto& bufferView = model.bufferViews[accessor.bufferView];
		const auto& buffer = model.buffers[bufferView.buffer];

		const float* data = reinterpret_cast<const float*>(&buffer.data[bufferView.byteOffset + accessor.byteOffset]);
		std::vector<glm::vec4> values;

		if (accessor.type == TINYGLTF_TYPE_VEC3) // Translation/Scale
		{
			for (size_t i = 0; i < accessor.count; ++i)
			{
				values.emplace_back(data[i * 3 + 0], data[i * 3 + 1], data[i * 3 + 2], 0.0f); // Pad with 0.0f for `w`
			}
		}
		else if (accessor.type == TINYGLTF_TYPE_VEC4) // Rotation
		{
			for (size_t i = 0; i < accessor.count; ++i)
			{
				values.emplace_back(data[i * 4 + 0], data[i * 4 + 1], data[i * 4 + 2], data[i * 4 + 3]);
			}
		}
		else
		{
			std::cerr << "Unsupported accessor type for keyframe values\n";
		}

		return values;
	}

	SubMesh GLBLoader::CreateSubMesh(const tinygltf::Model& model, 
		const std::vector<const tinygltf::Primitive*>& primitives, MaterialVertexAttributeKey key)
	{		
		std::vector<float> positions, normals, texCoords, tangents, texCoords2, weights;
		std::vector<uint32_t> indices;
		std::vector< uint16_t> joints;

		uint32_t indexOffset = 0;
		BatchLoadAttributes(model, primitives, positions, normals, texCoords, texCoords2, tangents, weights, joints, indices, indexOffset, key.attributesKey);

		GenerateMissingAttributes(positions, normals, texCoords, tangents, indices, key.attributesKey);

		Material* material;
		if (!model.materials.empty())
		{
			if (key.materialIndex < 0)
			{
				material = m_resourceLoader->GetDefaultMaterial();
			}
			else
			{
				material = ParseMaterial(model, model.materials[key.materialIndex], key.materialIndex).get();
				UpdateUVsFromScaleOffset(texCoords, material->scale, material->offset);
			}
		}
		else
		{
			material = m_resourceLoader->GetDefaultMaterial();
		}

		// Interleave vertex data
		std::vector<std::byte> interleavedVertexData;
		Geometry::GenerateInterleavedVertexData(positions, normals, texCoords, texCoords2, tangents, interleavedVertexData);

		auto& vao = material->useTransparency ? m_transparentVAOs[key.attributesKey] : m_staticVAOs[key.attributesKey];
		if (!vao)
		{
			//std::cerr << "Error: No VAO found for attributes key " << key.attributesKey << "\n";
			
			vao = std::make_shared<VertexArrayObject>("vao" + std::to_string(key.attributesKey));
			if (material->useTransparency)
			{
				m_transparentVAOs[key.attributesKey] = vao;
			}
			else
			{
				m_staticVAOs[key.attributesKey] = vao;
			}
			vao->SetVertexAttribKey(key.attributesKey);
		}

		auto& vbo = vao->GetVBO();
		auto& vdata = vbo.GetDataMutable();
		uint32_t vertexOffset = (uint32_t)vdata.size() / (CalculateStrideInBytes(key.attributesKey));
		vdata.insert(vdata.end(), interleavedVertexData.begin(), interleavedVertexData.end());

		auto& ibo = vao->GetIBO();
		auto& idata = ibo.GetDataMutable();
		uint32_t indexBase = (uint32_t)idata.size();
		idata.insert(idata.end(), indices.begin(), indices.end());

		SubMesh submesh;
		submesh.isStatic = true;
		submesh.aabb = CalculateAABB(positions);
		submesh.attribKey = key.attributesKey;
		submesh.materialHandle = material->GetHandle();
		submesh.command = 
		{
			.count = static_cast<uint32_t>(indices.size()),
			.instanceCount = 1,
			.firstIndex = indexBase,
			.baseVertex = vertexOffset,
			.baseInstance = 0
		};

		return submesh;
	}

	void GLBLoader::UpdateUVsFromScaleOffset(std::vector<float>& uvs, glm::vec2 scale, glm::vec2 offset)
	{
		for (auto i = 0; i < uvs.size(); i += 2)
		{
			uvs[i] *= scale.x;
			uvs[i + 1] *= scale.y;
			uvs[i] += offset.x;
			uvs[i + 1] += offset.y;
		}
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
			if (material->baseColorFactor.a > 0.0f && material->baseColorFactor.a < 1.0f)
			{
				material->useTransparency = true;
			}
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
			TexParams params = Texture::EmptyParams();
			material->baseColorTexture = ParseTexture(model, texName, std::string(BASE_COLOR_TEXTURE), textureIndex, params);

			loadKHRTextureTransform(gltfMaterial, material);
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
			TexParams params = Texture::EmptyParams();
			material->metallicRoughnessTexture = ParseTexture(model, texName, std::string(METALLIC_ROUGHNESS_TEXTURE), textureIndex, params);
		}

		// Parse normal texture
		constexpr const char* NORMAL_TEXTURE = "normalTexture";
		if (gltfMaterial.additionalValues.find(NORMAL_TEXTURE) != gltfMaterial.additionalValues.end())
		{
			int textureIndex = gltfMaterial.additionalValues.at(NORMAL_TEXTURE).TextureIndex();
			auto texName = gltfMaterial.name + std::to_string(texId++);
			TexParams params = Texture::EmptyParams();
			material->normalTexture = ParseTexture(model, texName, std::string(NORMAL_TEXTURE), textureIndex, params);
		}

		// Parse occlusion texture
		constexpr const char* OCCLUSION_TEXTURE = "occlusionTexture";
		if (gltfMaterial.additionalValues.find(OCCLUSION_TEXTURE) != gltfMaterial.additionalValues.end())
		{
			int textureIndex = gltfMaterial.additionalValues.at(OCCLUSION_TEXTURE).TextureIndex();
			auto texName = gltfMaterial.name + std::to_string(texId++);
			TexParams params = Texture::EmptyParams();
			material->occlusionTexture = ParseTexture(model, texName, std::string(OCCLUSION_TEXTURE), textureIndex, params);
		}

		// Parse emissive texture
		constexpr const char* EMISSIVE_TEXTURE = "emissiveTexture";
		if (gltfMaterial.additionalValues.find(EMISSIVE_TEXTURE) != gltfMaterial.additionalValues.end())
		{
			int textureIndex = gltfMaterial.additionalValues.at(EMISSIVE_TEXTURE).TextureIndex();
			auto texName = gltfMaterial.name + std::to_string(texId++);
			TexParams params = Texture::EmptyParams();
			material->emissiveTexture = ParseTexture(model, texName, std::string(EMISSIVE_TEXTURE), textureIndex, params);
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

		// Parse KHR_materials_transmission
		constexpr const char* KHR_MATERIALS_TRANSMISSION = "KHR_materials_transmission";
		if (gltfMaterial.extensions.find(KHR_MATERIALS_TRANSMISSION) != gltfMaterial.extensions.end())
		{
			const tinygltf::Value& transmission = gltfMaterial.extensions.at(KHR_MATERIALS_TRANSMISSION);
			if (transmission.Has("transmissionFactor") && transmission.Get("transmissionFactor").IsNumber())
			{
				material->transmissionFactor = static_cast<float>(transmission.Get("transmissionFactor").GetNumberAsDouble());
				if (material->transmissionFactor)
					material->useTransparency = true;
			}
			else
			{
				material->transmissionFactor = 0.0f; 
			}
		}
		else
		{
			material->transmissionFactor = 0.0f; 
		}

		// Parse KHR_materials_volume
		constexpr const char* KHR_MATERIALS_VOLUME = "KHR_materials_volume";
		if (gltfMaterial.extensions.find(KHR_MATERIALS_VOLUME) != gltfMaterial.extensions.end())
		{
			const tinygltf::Value& volume = gltfMaterial.extensions.at(KHR_MATERIALS_VOLUME);

			if (volume.Has("thicknessFactor") && volume.Get("thicknessFactor").IsNumber())
			{
				material->thickness = static_cast<float>(volume.Get("thicknessFactor").GetNumberAsDouble());
			}
			else
			{
				material->thickness = 0.0f; 
			}

			// Attenuation color
			if (volume.Has("attenuationColor") && volume.Get("attenuationColor").IsArray())
			{
				const auto& colorArray = volume.Get("attenuationColor");
				material->attenuationColor = glm::vec3(
					static_cast<float>(colorArray.Get(0).GetNumberAsDouble()),
					static_cast<float>(colorArray.Get(1).GetNumberAsDouble()),
					static_cast<float>(colorArray.Get(2).GetNumberAsDouble())
				);
			}
			else
			{
				material->attenuationColor = glm::vec3(1.0f); 
			}

			// Attenuation distance
			if (volume.Has("attenuationDistance") && volume.Get("attenuationDistance").IsNumber())
			{
				material->attenuationDistance = static_cast<float>(volume.Get("attenuationDistance").GetNumberAsDouble());
			}
			else
			{
				material->attenuationDistance = std::numeric_limits<float>::max(); 
			}
		}
		else
		{
			material->thickness = 0.0f;
			material->attenuationColor = glm::vec3(1.0f);
			material->attenuationDistance = std::numeric_limits<float>::max();
		}

		// Refraction Index (KHR_materials_ior)
		constexpr const char* KHR_MATERIALS_IOR = "KHR_materials_ior";
		if (gltfMaterial.extensions.find(KHR_MATERIALS_IOR) != gltfMaterial.extensions.end())
		{
			const tinygltf::Value& ior = gltfMaterial.extensions.at(KHR_MATERIALS_IOR);
			if (ior.Has("ior") && ior.Get("ior").IsNumber())
			{
				material->refractionIndex = static_cast<float>(ior.Get("ior").GetNumberAsDouble());
			}
			else
			{
				material->refractionIndex = 1.5f; // Default IOR for glass
			}
		}
		else
		{
			material->refractionIndex = 1.5f; // Default IOR for glass
		}

		// Alpha properties
		material->alphaMode = AlphaModeFromString(gltfMaterial.alphaMode);
		material->alphaCutoff = static_cast<float>(gltfMaterial.alphaCutoff);
		material->doubleSided = gltfMaterial.doubleSided;

		// Cache the material
		materialCache[matIdx] = material;

		return material;
	}

	std::shared_ptr<Texture> GLBLoader::ParseTexture(const tinygltf::Model& model, std::string& matName, const std::string& name, int textureIndex, TexParams overwriteParams)
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
		const std::string& finalName = matName + "_" + name;

		// Extract texture details
		uint32_t width = static_cast<uint32_t>(glbImageData.width);
		uint32_t height = static_cast<uint32_t>(glbImageData.height);
		int channels = glbImageData.component;
		ImageData imgData;
		imgData.width = width;
		imgData.height = height;
		imgData.channels = channels;
		imgData.data = glbImageData.image;

		auto params = Texture::DefaultParams(imgData.channels, false);
		auto newParams = Texture::OverwriteParams(params, overwriteParams);

		// Create a new texture
		auto jltexture = m_resourceLoader->CreateTexture(finalName, imgData, newParams);

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

	void GLBLoader::loadKHRTextureTransform(const tinygltf::Material& gltfMaterial, std::shared_ptr<Material> material)
	{
		auto& extensions = gltfMaterial.pbrMetallicRoughness.baseColorTexture.extensions;

		if (extensions.find("KHR_texture_transform") != extensions.end())
		{
			const tinygltf::Value& transform = extensions.at("KHR_texture_transform");

			if (transform.Has("scale") && transform.Get("scale").IsArray())
			{
				const auto& scaleArray = transform.Get("scale");
				bool isArray = scaleArray.IsArray();
				auto valX = static_cast<float>(scaleArray.Get(0).GetNumberAsDouble());
				auto valY = static_cast<float>(scaleArray.Get(1).GetNumberAsDouble());
				material->scale = glm::vec2(valX, valY);
			}
		}
	}

	SubMesh GLBLoader::CreateSubMeshAnim(const tinygltf::Model& model, const std::vector<const tinygltf::Primitive*>& primitives, MaterialVertexAttributeKey key)
	{
		std::vector<float> positions, normals, texCoords, tangents, texCoords2, weights;
		std::vector<uint32_t> indices;
		std::vector< uint16_t> joints;
		
		uint32_t indexOffset = 0;
		BatchLoadAttributes(model, primitives, positions, normals, texCoords, texCoords2, tangents, weights, joints, indices, indexOffset, key.attributesKey);

		GenerateMissingAttributes(positions, normals, texCoords, tangents, indices, key.attributesKey);

		Material* material;
		if (!model.materials.empty())
		{
			if (key.materialIndex < 0)
			{
				material = m_resourceLoader->GetDefaultMaterial();
			}
			else
			{
				material = ParseMaterial(model, model.materials[key.materialIndex], key.materialIndex).get();
				UpdateUVsFromScaleOffset(texCoords, material->scale, material->offset);
			}
		}
		else
		{
			material = m_resourceLoader->GetDefaultMaterial();
		}

		// Interleave vertex data
		std::vector<std::byte> interleavedVertexData;
		Geometry::GenerateInterleavedVertexData(positions, normals, texCoords, tangents, weights, joints, interleavedVertexData);

		auto& vao = m_skinnedMeshVAOs[key.attributesKey];
		if (!vao)
		{
			vao = std::make_shared<VertexArrayObject>("vao" + std::to_string(key.attributesKey));
			m_skinnedMeshVAOs[key.attributesKey] = vao;
			vao->SetVertexAttribKey(key.attributesKey);
		}

		auto& vbo = vao->GetVBO();
		auto& vdata = vbo.GetDataMutable();
		uint32_t vertexOffset = (uint32_t)vdata.size() / (CalculateStrideInBytes(key.attributesKey));
		vdata.insert(vdata.end(), interleavedVertexData.begin(), interleavedVertexData.end());

		auto& ibo = vao->GetIBO();
		auto& idata = ibo.GetDataMutable();
		uint32_t indexBase = (uint32_t)idata.size();
		idata.insert(idata.end(), indices.begin(), indices.end());

		SubMesh submesh;
		submesh.isStatic = false;
		submesh.aabb = CalculateAABB(positions);
		submesh.attribKey = key.attributesKey;
		submesh.materialHandle = material->GetHandle();
		submesh.command = 
		{
			.count = static_cast<uint32_t>(indices.size()),
			.instanceCount = 1,
			.firstIndex = indexBase,
			.baseVertex = vertexOffset,
			.baseInstance = 0
		};

		return submesh;
	}

	void GLBLoader::BatchLoadAttributes(const tinygltf::Model& model,
		const std::vector<const tinygltf::Primitive*>& primitives,
		std::vector<float>& positions,
		std::vector<float>& normals,
		std::vector<float>& texCoords,
		std::vector<float>& texCoords2,
		std::vector<float>& tangents, 
		std::vector<float>& weights,      
		std::vector<uint16_t>& joints,
		std::vector<uint32_t>& indices,
		uint32_t& indexOffset, 
		VertexAttribKey key)
	{
		if (primitives.size() > 1)
			std::cout << "check here, primitive count > 1";
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
			if (HasVertexAttribKey(key, AttributeType::WEIGHT_0))  // New: Load WEIGHTS_0
				LoadWeightAttribute(model, *primitive, weights);
			if (HasVertexAttribKey(key, AttributeType::JOINT_0))   // New: Load JOINTS_0
				LoadJointAttribute(model, *primitive, joints);

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

	std::vector<Animation*> GLBLoader::GetAnimationsFromSkeleton(int gltfSkeletonRoot)
	{
		std::vector<Animation*> animations;
		auto& resources = m_resourceLoader->GetAnimationManager()->GetResources();
		for (auto& anim : resources)
		{
			for (auto& channel : anim.second->GetChannels())
			{
				if (channel.GetTargetNode() == gltfSkeletonRoot)
				{
					animations.push_back(anim.second.get());
					break;
				}
			}
		}
		return animations;
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

	void GLBLoader::LoadWeightAttribute(const tinygltf::Model& model, const tinygltf::Primitive& primitive, std::vector<float>& weights)
	{
		const auto& weightAttr = primitive.attributes.find("WEIGHTS_0");
		if (weightAttr != primitive.attributes.end())
		{
			const tinygltf::Accessor& accessor = model.accessors[weightAttr->second];
			const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
			const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];

			const float* data = reinterpret_cast<const float*>(&buffer.data[bufferView.byteOffset + accessor.byteOffset]);
			size_t count = accessor.count * 4; // WEIGHTS_0 is typically a vec4
			weights.insert(weights.end(), data, data + count);
		}
	}

	void GLBLoader::LoadJointAttribute(const tinygltf::Model& model, const tinygltf::Primitive& primitive, std::vector<uint16_t>& joints)
	{
		const auto& jointAttr = primitive.attributes.find("JOINTS_0");
		if (jointAttr != primitive.attributes.end())
		{
			const tinygltf::Accessor& accessor = model.accessors[jointAttr->second];
			const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
			const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];

			const uint16_t* data = reinterpret_cast<const uint16_t*>(&buffer.data[bufferView.byteOffset + accessor.byteOffset]);
			size_t count = accessor.count * 4; // JOINTS_0 is typically a vec4 of uint16
			joints.insert(joints.end(), data, data + count);
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