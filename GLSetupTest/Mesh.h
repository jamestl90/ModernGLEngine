#ifndef MESH_H
#define MESH_H

#include "Types.h"
#include "Primitives.h"
#include "VertexBuffers.h"
#include "Resource.h"

namespace JLEngine
{
	class Graphics;

	class Mesh : public Resource
	{
	public:
		Mesh(uint32 handle, string& name);
		Mesh(uint32 handle, string& name, string& path);

		~Mesh();

		void Init(Graphics* graphics);

		void UnloadFromGraphics();

		void SetVao(uint32 id) { m_vao = id; }

		uint32 GetVaoId() { return m_vao; }

		void SetVertexBuffer(VertexBuffer& vbo);

		VertexBuffer& GetVertexBuffer();

		void SetIndexBuffer(IndexBuffer& ibo);

		void SetHasIndices(bool hasIndices) { m_hasIndices = hasIndices; }

		bool HasIndices() { return m_hasIndices; }

		IndexBuffer& GetIndexBuffer();

		uint32 GetMaterialHandle() { return m_materialId; }

		void SetMaterial(uint32 handle) { m_materialId = handle; }

		const AABB& GetAABB() { return m_aabb; }

		AABB CalculateAABB();

	private:

		Graphics* m_graphics;

		AABB m_aabb;

		bool m_hasIndices;

		uint32 m_materialId;

		uint32 m_vao;

		IndexBuffer m_ibo;

		VertexBuffer m_vbo;
	};
}

#endif