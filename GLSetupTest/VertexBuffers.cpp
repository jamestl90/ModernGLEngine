#include "VertexBuffers.h"
#include <glad/glad.h>

namespace JLEngine
{
	VertexBuffer::VertexBuffer(int type, int data, int draw )
		: GraphicsBuffer(type, data, draw), m_stride(0)
	{
	}

	VertexBuffer::VertexBuffer( std::vector<float>& vertices, int type, int data, int draw )
		: Buffer(vertices), GraphicsBuffer(type, data, draw), m_stride(0)
	{
	}

	VertexBuffer::~VertexBuffer()
	{
	}

	void VertexBuffer::Add( glm::vec3& val )
	{
		m_buffer.push_back(val.x);
		m_buffer.push_back(val.y);
		m_buffer.push_back(val.z);
	}

	void VertexBuffer::AddAttribute(const VertexAttribute& attrib)
	{
		m_attributes.insert(attrib);
	}

	uint32 VertexBuffer::GetStride()
	{
		return m_stride;
	}

	uint32 VertexBuffer::SizeInBytes()
	{
		return size_f * (uint32)Size();
	}

	const std::set<VertexAttribute>& VertexBuffer::GetAttributes() const
	{
		return m_attributes;
	}

	void VertexBuffer::CalcStride()
	{
		m_stride = 0;
		for (auto it = m_attributes.begin(); it != m_attributes.end(); ++it)
		{
			m_stride += (*it).m_count;
		}
		//m_stride = m_attributes.size() * 3;
	}

	IndexBuffer::IndexBuffer( std::vector<uint32>& indices, int type, int data, int draw ) : Buffer(indices), GraphicsBuffer(type, data, draw)
	{
	}

	IndexBuffer::IndexBuffer(int type, int data, int draw )
	{

	}

	IndexBuffer::~IndexBuffer()
	{
	}

	GraphicsBuffer::GraphicsBuffer(int type, int data, int drawType ) 
		: m_type(type), m_dataType(data), m_drawType(drawType), m_id(0)
	{
	}

	GraphicsBuffer::GraphicsBuffer() 
		: m_type(GL_ARRAY_BUFFER), m_dataType(GL_FLOAT), m_drawType(GL_STATIC_DRAW), m_id(0)
	{

	}

	std::pair<glm::vec3, glm::vec3> findMaxMinPair(VertexBuffer& vbo)
	{
		glm::vec3 max = glm::vec3(vbo.GetBuffer().at(0), vbo.GetBuffer().at(1), vbo.GetBuffer().at(2));
		glm::vec3 min = glm::vec3(vbo.GetBuffer().at(6), vbo.GetBuffer().at(7), vbo.GetBuffer().at(8));

		for (uint32 i = 0; i < vbo.Size(); i += vbo.GetStride())
		{
			float x = vbo.GetBuffer().at(i);
			float y = vbo.GetBuffer().at(i + 1);
			float z = vbo.GetBuffer().at(i + 2);

			max.x = max.x > x ? max.x : x;
			max.y = max.y > y ? max.y : y;
			max.z = max.z > z ? max.z : z;

			min.x = min.x < x ? min.x : x;
			min.y = min.y < y ? min.y : y;
			min.z = min.z < z ? min.z : z;
		}
		return std::pair<glm::vec3, glm::vec3>(max, min);
	}
}