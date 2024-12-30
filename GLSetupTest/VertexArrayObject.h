#ifndef VERTEX_ARRAY_OBJECT_H
#define VERTEX_ARRAY_OBJECT_H

#include "VertexBuffers.h"
#include "VertexStructures.h"
#include "Resource.h"

#include <memory>
#include <string>

namespace JLEngine
{
	class VertexArrayObject
	{
	public:
		VertexArrayObject();
		VertexArrayObject(std::string name)
			: m_gpuID(0), m_key(0), m_stride(0), m_vbo("vbo:" + name), m_ibo("ibo:" + name) {}

		void SetVertexBuffer(const VertexBuffer vertexBuffer) { m_vbo = vertexBuffer; }
		void SetIndexBuffer(const IndexBuffer indexBuffer) { m_ibo = indexBuffer; m_hasIndices = true; }

		VertexBuffer& GetVBO() { return m_vbo; }
		IndexBuffer& GetIBO() { return m_ibo; }

		VertexAttribKey GetAttribKey() { return m_key; }
		void SetVertexAttribKey(VertexAttribKey key) { m_key = key; }
		void AddAttribute(AttributeType type) {	AddToVertexAttribKey(m_key, type); }
		bool HasAttribute(AttributeType type) {	return HasVertexAttribKey(m_key, type);	}

		void CalcStride() { m_stride = CalculateStride(this); }
		const uint32_t GetStride() const { return m_stride; }
		
		bool HasIndices() { return m_ibo.GetDataImmutable().size() > 0; }

		void SetPosCount(int count) { m_posCount = count; }
		int GetPosCount() const { return m_posCount; }

		void SetGPUID(uint32_t gpuid) { m_gpuID = gpuid; }
		uint32_t GetGPUID() const { return m_gpuID; }

	private:

		int m_posCount = 3;
		int m_normCount = 3;
		int m_uvCount = 2;
		int m_tangentCount = 4;
		int m_uv2Count = 2;
		int m_vertColorCount = 4;

		uint32_t m_gpuID;

		bool m_hasIndices = false;

		VertexAttribKey m_key;
		uint32_t m_stride;

		VertexBuffer m_vbo;
		IndexBuffer m_ibo;
	};
}

#endif