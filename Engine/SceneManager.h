#ifndef SCENE_MANAGER_H
#define SCENE_MANAGER_H

#include <vector>
#include <functional>
#include <string>
#include <algorithm>
#include <memory>
#include <glm/glm.hpp>

#include "Node.h"
#include "ResourceLoader.h"

namespace JLEngine
{
	class SceneManager
	{
	public:
		SceneManager() : m_sceneRoot(nullptr), m_resourceLoader(nullptr) {}
		SceneManager(Node* node, ResourceLoader* resourceLoader) : m_sceneRoot(node), m_resourceLoader(resourceLoader) {}
		~SceneManager() {}

		void SetRoot(std::shared_ptr<Node>& root) { m_sceneRoot = root; }
		std::shared_ptr<Node>& GetRoot() { return m_sceneRoot; }

		void ForceUpdate()
		{
			m_nonInstancedStatic.clear();
			m_nonInstancedDynamic.clear();
			m_instancedStatic.clear();
			m_transparentObjects.clear();
			m_skinnedAnimControllers.clear();
			m_rigidAnimControllers.clear();
			m_submeshList.clear();
			m_skinnedAnimControllers.clear();
			m_rigidAnimControllers.clear();

			std::function<void(Node*)> populateLists = [&](Node* node)
				{
					if (node->IsAnimated && node->GetTag() != NodeTag::Mesh) // animated node, but no mesh
					{
						m_rigidAnimControllers.push_back(std::make_pair(node->animController, node));
					}
					if (node->GetTag() == NodeTag::Light)
					{
						m_lights.try_emplace(node->name, node);
					}
					if (node->GetTag() == NodeTag::Mesh)
					{
						auto matMgr = m_resourceLoader->GetMaterialManager();

						auto& submeshes = node->mesh->GetSubmeshes();
						for (auto i = 0; i < submeshes.size(); i++)
						{
							auto& submesh = node->mesh->GetSubmesh(i); 
							m_submeshList.push_back(std::make_pair(submesh, node));
							auto isInstanced = submesh.instanceTransforms != nullptr;
							auto isStatic = (submesh.flags & SubmeshFlags::STATIC) != 0;
							auto isSkinned = (submesh.flags & SubmeshFlags::SKINNED) != 0;
							auto usesTransparency = (submesh.flags & SubmeshFlags::USES_TRANSPARENCY) != 0;
							auto isAnimated = (submesh.flags & SubmeshFlags::ANIMATED) != 0;
							auto mat = matMgr->Get(submesh.materialHandle);

							if (isInstanced && !usesTransparency && isStatic)
							{
								auto key = MakeKey(node->mesh->GetName(), submesh);
								if (m_instancedStatic.find(key) == m_instancedStatic.end())
								{
									m_instancedStatic[key] = std::make_pair(submesh, node);
								}
							}
							else if (usesTransparency) // not currently handling transparent/skinned, but will eventually 
							{
								m_transparentObjects.push_back(std::make_pair(submesh, node));
							}
							else if (isStatic && !isAnimated)
							{
								m_nonInstancedStatic.push_back(std::make_pair(submesh, node));
							}
							else if (isInstanced && isSkinned)
							{
								m_skinnedAnimControllers.push_back(std::make_pair(node->animController, node));
								auto key = MakeKey(node->mesh->GetName(), submesh);
								if (m_instancedDynamic.find(key) == m_instancedDynamic.end())
								{
									m_instancedDynamic[key] = std::make_pair(submesh, node);
								}
							}
							else if (isSkinned && isAnimated)
							{
								m_skinnedAnimControllers.push_back(std::make_pair(node->animController, node));
								m_nonInstancedDynamic.push_back(std::make_pair(submesh, node));
							}
							else if (isAnimated)
							{
								// non skinned animations
								m_rigidAnimControllers.push_back(std::make_pair(node->animController, node));
								m_animatedRigidObjects.push_back(std::make_pair(submesh, node));
							}
						}
					}

					for (auto& child : node->children)
					{
						populateLists(child.get());
					}
				};
			populateLists(m_sceneRoot.get());
		}

		std::unordered_map<std::string, Node*>& GetLightNodes() { return m_lights; }

		std::vector<std::pair<JLEngine::SubMesh, Node*>>& GetSubmeshes()
		{
			return m_submeshList;
		}

		std::vector<std::pair<SubMesh, Node*>>& GetNonInstancedStatic()
		{
			return m_nonInstancedStatic;
		}

		std::vector<std::pair<SubMesh, Node*>>& GetNonInstancedDynamic()
		{
			return m_nonInstancedDynamic;
		}

		std::vector<std::pair<SubMesh, Node*>>& GetRigidAnimated()
		{
			return m_animatedRigidObjects;
		}

		std::unordered_map<std::string, std::pair<SubMesh, Node*>>& GetInstancedStatic()
		{
			return m_instancedStatic;
		}

		std::unordered_map<std::string, std::pair<SubMesh, Node*>>& GetInstancedDynamic()
		{
			return m_instancedDynamic;
		}

		std::vector<std::pair<SubMesh, Node*>>& GetTransparent()
		{
			return m_transparentObjects;
		}

		std::vector<std::pair<std::shared_ptr<AnimationController>, Node*>>& GetSkinnedAnimationControllers()
		{
			return m_skinnedAnimControllers;
		}

		std::vector<std::pair<std::shared_ptr<AnimationController>, Node*>>& GetRigidAnimationControllers()
		{
			return m_rigidAnimControllers;
		}

		void SortStaticFrontToBack(glm::vec3& eyePos)
		{
			std::sort(m_nonInstancedStatic.begin(), m_nonInstancedStatic.end(),
				[&](const auto& a, const auto& b)
				{
					return glm::distance2(eyePos, a.second->translation) < glm::distance2(eyePos, b.second->translation);
				});
		}

		void SortDynamicFrontToBack(glm::vec3& eyePos)
		{
			std::sort(m_nonInstancedDynamic.begin(), m_nonInstancedDynamic.end(),
				[&](const auto& a, const auto& b)
				{
					return glm::distance2(eyePos, a.second->translation) < glm::distance2(eyePos, b.second->translation);
				});
		}

		void SortTransparentBackToFront(const glm::vec3& eyePos)
		{
			std::sort(m_transparentObjects.begin(), m_transparentObjects.end(),
				[&](const auto& a, const auto& b)
				{
					return glm::distance2(eyePos, a.second->translation) > glm::distance2(eyePos, b.second->translation);
				});
		}

		void RebuildTransparentDrawCommands(std::unordered_map<VertexAttribKey, VAOResource>& transparentResources,
									  std::unordered_map<uint32_t, size_t>& materialIDMap,
									  ShaderStorageBuffer<PerDrawData> ssboTransparentPerDraw)
		{
			auto matMgr = m_resourceLoader->GetMaterialManager();
			for (auto& item : transparentResources)
			{
				auto& vaoRes = item.second;
				auto& drawBuffer = vaoRes.drawBuffer;
				drawBuffer->ClearCommands();

				for (auto& transObj : m_transparentObjects)
				{
					PerDrawData pdd;
					pdd.materialID = (int)materialIDMap[transObj.first.materialHandle];
					pdd.modelMatrix = transObj.second->GetGlobalTransform();
					auto mat = matMgr->Get(transObj.first.materialHandle);

					ssboTransparentPerDraw.AddData(pdd);
					transparentResources[transObj.first.attribKey].drawBuffer->AddDrawCommand(transObj.first.command);
				}
				Graphics::UploadToGPUBuffer(drawBuffer->GetGPUBuffer(), drawBuffer->GetDataImmutable(), 0);
			}
			Graphics::UploadToGPUBuffer(ssboTransparentPerDraw.GetGPUBuffer(), ssboTransparentPerDraw.GetDataImmutable(), 0);
		}

	private:

		std::shared_ptr<Node> m_sceneRoot;
		std::vector<std::pair<SubMesh, Node*>> m_nonInstancedStatic;  // static meshes
		std::vector<std::pair<SubMesh, Node*>> m_animatedRigidObjects; // rigid animations
		std::vector<std::pair<SubMesh, Node*>> m_nonInstancedDynamic; // skinned meshes
		std::vector<std::pair<SubMesh, Node*>> m_transparentObjects; // transparent meshes		
		std::unordered_map<std::string, Node*> m_lights;
		std::unordered_map<std::string, std::pair<SubMesh, Node*>> m_instancedStatic;	// instanced static meshes
		std::unordered_map<std::string, std::pair<SubMesh, Node*>> m_instancedDynamic; // instanced skinned meshes
		std::vector<std::pair<SubMesh, Node*>> m_submeshList;
		std::vector<std::pair<std::shared_ptr<AnimationController>, Node*>> m_skinnedAnimControllers;
		std::vector<std::pair<std::shared_ptr<AnimationController>, Node*>> m_rigidAnimControllers;

		ResourceLoader* m_resourceLoader;
	};
}

#endif