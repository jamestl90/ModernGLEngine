#include "Mesh.h"
#include "Graphics.h"

namespace JLEngine
{
	Mesh::Mesh(uint32 handle, const string& name)
		: Resource(handle, name), m_vao(0), m_graphics(nullptr), m_hasIndices(false)
	{
	}

	Mesh::~Mesh()
	{
		UnloadFromGraphics();

		m_vbo.Clear();

		if (m_ibos.size() > 0)
		{
			for (auto& ibo : m_ibos)
			{
				ibo.Clear();
			}
			m_ibos.clear();
		}
	}

	AABB Mesh::CalculateAABB()
	{
		std::pair<glm::vec3, glm::vec3> maxMin;

		maxMin = findMaxMinPair(m_vbo);

		m_aabb.max = maxMin.first;
		m_aabb.min = maxMin.second;

		return m_aabb;
	}

	void Mesh::UploadToGPU(Graphics* graphics, bool freeData)
	{
		m_graphics = graphics;

		m_graphics->CreateMesh(this);

		if (m_ibos.size() != 0)
		{
			m_vbo.Clear();
		}

		if (m_vbo.GetBuffer().size() > 50.0f)
		{
			CalculateAABB();
		}
	}

	void Mesh::SetVertexBuffer( VertexBuffer& vbo )
	{
		m_vbo = vbo;
	}

	VertexBuffer& Mesh::GetVertexBuffer()
	{
		return m_vbo;
	}

	void Mesh::AddIndexBuffer( IndexBuffer& ibo )
	{
		m_ibos.push_back(ibo);
		m_hasIndices = true;
	}

	IndexBuffer& Mesh::GetIndexBuffer()
	{
		return m_ibos[0];
	}

	IndexBuffer& Mesh::GetIndexBufferAt(int idx)
	{
		return m_ibos[idx];
	}

	void Mesh::UnloadFromGraphics()
	{
		m_graphics->DisposeMesh(this);
	}
}