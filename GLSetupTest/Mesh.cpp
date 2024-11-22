#include "Mesh.h"
#include "Graphics.h"

namespace JLEngine
{
	Mesh::Mesh( uint32 handle, string& name, string& path )
		: GraphicsResource(handle, name, path), m_materialId(-1)
	{
	}

	Mesh::~Mesh()
	{
		UnloadFromGraphics();

		m_vbo.Clear();
		m_ibo.Clear();
	}

	AABB Mesh::CalculateAABB()
	{
		std::pair<glm::vec3, glm::vec3> maxMin;

		maxMin = findMaxMinPair(m_vbo);

		m_aabb.max = maxMin.first;
		m_aabb.min = maxMin.second;

		return m_aabb;
	}

	void Mesh::Init(Graphics* graphics)
	{
		m_graphics = graphics;

		// saves us from doing those 4 lines 
		// of code commented out below
		m_graphics->CreateMesh(this);

		/*
		graphics->createVertexArrayObject(m_vao);
		graphics->bindVertexArrayObject(m_vao);
		graphics->BuildVertexBuffer(m_vbo);
		graphics->BuildIndexBuffer(m_ibo);
		*/

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

	void Mesh::SetIndexBuffer( IndexBuffer& ibo )
	{
		m_ibo = ibo;
		m_hasIndices = true;
	}

	IndexBuffer& Mesh::GetIndexBuffer()
	{
		return m_ibo;
	}

	void Mesh::UnloadFromGraphics()
	{
		m_graphics->DisposeMesh(this);
	}
}