#include "VertexBuffers.h"
#include <glad/glad.h>

namespace JLEngine
{
	VertexBuffer::VertexBuffer(uint32_t type, uint32_t data, uint32_t draw )
		: GraphicsBuffer(type, data, draw)
	{
	}

	VertexBuffer::VertexBuffer( std::vector<float>& vertices, uint32_t type, uint32_t data, uint32_t draw )
		: Buffer(vertices), GraphicsBuffer(type, data, draw)
	{
	}

	VertexBuffer::~VertexBuffer()
	{
	}

	uint32_t VertexBuffer::SizeInBytes()
	{
		return size_f * (uint32_t)Size();
	}

	IndexBuffer::IndexBuffer( std::vector<uint32_t>& indices, uint32_t type, uint32_t data, uint32_t draw )
		: Buffer(indices), GraphicsBuffer(type, data, draw)
	{
	}

	IndexBuffer::IndexBuffer(uint32_t type, uint32_t data, uint32_t draw )
		: GraphicsBuffer(type, data, draw)
	{

	}

	IndexBuffer::~IndexBuffer()
	{
	}

	GraphicsBuffer::GraphicsBuffer(uint32_t type, uint32_t data, uint32_t drawType )
		: m_type(type), m_dataType(data), m_drawType(drawType), m_id(0)
	{
	}

	GraphicsBuffer::GraphicsBuffer() 
		: m_type(GL_ARRAY_BUFFER), m_dataType(GL_FLOAT), m_drawType(GL_STATIC_DRAW), m_id(0)
	{

	}
}