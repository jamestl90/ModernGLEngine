#ifndef VERTEX_BUFFERS_H
#define VERTEX_BUFFERS_H

#include "VertexStructures.h"
#include "Buffer.h"

#include <glm/glm.hpp>
#include <set>
#include <vector>

using std::pair;
using std::vector;

namespace JLEngine
{
	class GraphicsBuffer 
	{
	public:
		GraphicsBuffer();

		GraphicsBuffer(int type, int data, int drawType);

		void SetId(uint32& id) { m_id = id; }

		uint32 GetId() const { return m_id; }

		void SetType(int type) { m_type = type; }

		void SetDataType(int dataType) { m_dataType = dataType; }

		void SetDrawType(int drawType) { m_drawType = drawType; }

		int Type() const { return m_type; }

		int DataType() const { return m_dataType; }

		int DrawType() const { return m_drawType; }

	protected:

		int m_type;
		int m_dataType;
		int m_drawType;

		uint32 m_id;
	};

	class IndexBuffer : public Buffer<uint32>, public GraphicsBuffer
	{
	public:
		IndexBuffer() {}

		IndexBuffer(int type, int data, int draw);

		IndexBuffer(vector<uint32>& indices, int type, int data, int draw);

		~IndexBuffer();
	};

	class VertexBuffer : public Buffer<float>, public GraphicsBuffer
	{
	public:
		VertexBuffer() : m_stride(0) {}

		VertexBuffer(int type, int data, int draw);

		VertexBuffer(vector<float>& vertices, int type, int data, int draw);

		~VertexBuffer();

		void Add(glm::vec3& val);

		void AddAttribute(VertexAttribute& attrib);

		void CalcStride();

		const std::set<VertexAttribute>& GetAttributes() const;

		uint32 GetStride();

		uint32 SizeInBytes();

	private:

		std::set<VertexAttribute> m_attributes;

		uint32 m_stride;
	};

	pair<glm::vec3, glm::vec3> findMaxMinPair(VertexBuffer&  vertices);
}

#endif