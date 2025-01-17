#ifndef MESH_H
#define MESH_H

#include "Resource.h"
#include "IndirectDrawBuffer.h"
#include "CollisionShapes.h"
#include "AnimData.h"

#include <memory>
#include <vector>
#include <string>

namespace JLEngine
{
	class Node;

	struct SubMesh
	{
		AABB aabb;
		bool isStatic = true;
		std::shared_ptr<std::vector<Node*>> instanceTransforms;
		uint32_t attribKey;
		uint32_t materialHandle;
		DrawIndirectCommand command;
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

		Skeleton& GetSkeleton() { return m_skeleton; }
		std::vector<glm::mat4>& GetInverseBindMatrices() { return m_inverseBindMatrices; }
		void SetInverseBindMatrix(int i, glm::mat4& matrix)
		{
			if (m_inverseBindMatrices.capacity() < i)
			{
				m_inverseBindMatrices.reserve(i + 1);
			}
			m_inverseBindMatrices[i] = matrix;
		}
		void SetSkeleton(Skeleton&& skeleton) { m_skeleton = std::move(skeleton); }
		void AddInverseBindMatrix(glm::mat4& matrix) { m_inverseBindMatrices.push_back(matrix); }

	private:	

		Skeleton m_skeleton;
		std::vector<glm::mat4> m_inverseBindMatrices;
		std::vector<SubMesh> m_subMeshes;	
	};
}

#endif