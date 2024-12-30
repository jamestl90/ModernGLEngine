#ifndef MESH_H
#define MESH_H

#include "Resource.h"
#include "IndirectDrawBuffer.h"
#include "CollisionShapes.h"

#include <memory>
#include <vector>
#include <string>

namespace JLEngine
{
	struct SubMesh
	{
		AABB aabb;
		uint32_t attribKey;
		uint32_t materialHandle;
		DrawIndirectCommand command;
	};

	class Mesh : public Resource
	{
	public:
		Mesh(const string& name);
		~Mesh();

		void AddSubmesh(const SubMesh& submesh) { m_subMeshes.push_back(submesh); }
		std::vector<SubMesh>& GetSubmeshes() { return m_subMeshes; }

		bool IsStatic() const { return m_isStatic; }
		void SetStatic(bool flag) { m_isStatic = flag; }

	private:	

		bool m_isStatic = true;
		std::vector<SubMesh> m_subMeshes;		
	};
}

#endif