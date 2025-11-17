#ifndef MESH_H
#define MESH_H

#include "Resource.h"
#include "IndirectDrawBuffer.h"
#include "CollisionShapes.h"
#include "AnimData.h"

#include <memory>
#include <vector>
#include <string>
#include "AnimationController.h"

namespace JLEngine
{
	class Node;

	enum SubmeshFlags : uint32_t
	{
		NONE				= 0,
		STATIC				= 1 << 0,
		SKINNED				= 1 << 1,
		INSTANCED			= 1 << 2,
		ANIMATED			= 1 << 4,
		USES_TRANSPARENCY	= 1 << 5,
		USE_SKELETON		= 1 << 6
	};

	struct SubMesh
	{
		uint32_t flags = SubmeshFlags::NONE;
		AABB aabb{};
		std::shared_ptr<std::vector<Node*>> instanceTransforms{};
		uint32_t attribKey = 0;
		uint32_t materialHandle = 0;
		DrawIndirectCommand command{};
	};

	std::string MakeKey(const std::string& meshName, const SubMesh& subMesh);

	class Mesh : public Resource
	{
	public:
		Mesh(const string& name);
		~Mesh();

		void AddSubmesh(const SubMesh& submesh) { m_subMeshes.push_back(submesh); }
		std::vector<SubMesh>& GetSubmeshes() { return m_subMeshes; }
		SubMesh& GetSubmesh(int idx) { return m_subMeshes[idx]; }

		std::shared_ptr<Skeleton>& GetSkeleton() { return m_skeleton; }
		std::vector<glm::mat4>& GetInverseBindMatrices() { return m_inverseBindMatrices; }
		void SetInverseBindMatrix(int i, glm::mat4& matrix)
		{
			if (m_inverseBindMatrices.capacity() < i)
			{
				m_inverseBindMatrices.reserve(i + 1);
			}
			m_inverseBindMatrices[i] = matrix;
		}
		void SetSkeleton(std::shared_ptr<Skeleton>& skeleton)
		{ 
			m_skeleton = std::move(skeleton);
		}
		void AddInverseBindMatrix(glm::mat4& matrix) { m_inverseBindMatrices.push_back(matrix); }

		Node* node;

	private:	

		std::shared_ptr<Skeleton> m_skeleton;
		std::vector<glm::mat4> m_inverseBindMatrices;
		std::vector<SubMesh> m_subMeshes;	
	};
}

#endif