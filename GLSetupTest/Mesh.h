#ifndef MESH_H
#define MESH_H

#include "Types.h"
#include "CollisionShapes.h"
#include "VertexBuffers.h"
#include "Resource.h"
#include "Material.h"

namespace JLEngine
{
	class Graphics;

	class Mesh : public Resource
	{
	public:
		Mesh(uint32 handle, const string& name);
		~Mesh();

		void UploadToGPU(Graphics* graphics, bool freeData);
		void UnloadFromGraphics();

		void SetVao(uint32 id) { m_vao = id; }
		uint32 GetVaoId() const { return m_vao; }
		void SetVertexBuffer(VertexBuffer& vbo);
		VertexBuffer& GetVertexBuffer();

		void AddIndexBuffer(IndexBuffer& ibo);
		void SetHasIndices(bool hasIndices) { m_hasIndices = hasIndices; }
		bool HasIndices() const { return m_hasIndices; }
		IndexBuffer& GetIndexBuffer();
		IndexBuffer& GetIndexBufferAt(int idx);
		std::vector<IndexBuffer>& GetIndexBuffers() { return m_ibos; }

		std::vector<Material*>& GetMaterials() { return m_materials; }
		Material* GetMaterialAt(int idx) { return m_materials[idx]; }
		void AddMaterial(Material* material) { m_materials.push_back(material); }

		const AABB& GetAABB() { return m_aabb; }
		AABB CalculateAABB();

	private:

		Graphics* m_graphics;

		AABB m_aabb;

		bool m_hasIndices;

		std::vector<Material*> m_materials;

		uint32 m_vao;

		std::vector<IndexBuffer> m_ibos;

		VertexBuffer m_vbo;
	};
}

#endif