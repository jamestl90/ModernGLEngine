#ifndef VERTEX_ARRAY_OBJECT_H
#define VERTEX_ARRAY_OBJECT_H

#include "VertexBuffers.h"
#include "VertexStructures.h"

#include <memory>

namespace JLEngine
{
	class VertexArrayObject
	{
	public:
		VertexArrayObject();

		const uint32_t GetGPUID() const { return m_GPUID; }
		void SetGPUID(uint32_t id) { m_GPUID = id; }

		void SetVertexBuffer(const VertexBuffer vertexBuffer) { m_vbo = vertexBuffer; }
		void SetIndexBuffer(const IndexBuffer indexBuffer) { m_ibo = indexBuffer; }

		VertexBuffer& GetVBO() { return m_vbo; }
		IndexBuffer& GetIBO() { return m_ibo; }

		VertexAttribKey GetAttribKey() { return m_key; }
		void SetVertexAttribKey(VertexAttribKey key) { m_key = key; }
		void AddAttribute(AttributeType type) {	AddToVertexAttribKey(m_key, type); }
		bool HasAttribute(AttributeType type) {	return HasVertexAttribKey(m_key, type);	}

		void CalcStride() { m_stride = CalculateStride(m_key); }
		const uint32_t GetStride() const { return m_stride; }

	private:

		VertexAttribKey m_key;
		uint32_t m_GPUID;
		uint32_t m_stride;

		VertexBuffer m_vbo;
		IndexBuffer m_ibo;
	};
}

#endif