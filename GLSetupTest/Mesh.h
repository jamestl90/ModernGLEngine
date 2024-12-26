#ifndef MESH_H
#define MESH_H

#include "Types.h"
#include "CollisionShapes.h"
#include "VertexBuffers.h"
#include "Resource.h"

#include <memory>
#include <vector>
#include <string>
#include "VertexBuffers.h"
#include "IndirectDrawBuffer.h"

namespace JLEngine
{
	class GraphicsAPI;
	class VertexArrayObject;

	struct SubMesh
	{
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

		bool m_isStatic = false;
		std::vector<SubMesh> m_subMeshes;		
	};
}

#endif