#ifndef VERTEX_BUFFERS_H
#define VERTEX_BUFFERS_H

#include "VertexStructures.h"
#include "Buffer.h"

#include <glm/glm.hpp>
#include <set>
#include <vector>
#include <glad/glad.h>

using std::pair;
using std::vector;

namespace JLEngine
{
	//enum class VertexFormat
	//{
	//	POS_NORMAL,
	//	POS_NORMAL_UV_TANGENT,
	//	POS_UV
	//};

	class GraphicsBuffer 
	{
	public:
		GraphicsBuffer();

		GraphicsBuffer(uint32_t type, uint32_t data, uint32_t drawType);

		void SetId(uint32_t& id) { m_id = id; m_uploadedToGPU = true; }

		uint32_t GetId() const { return m_id; }

		void SetType(uint32_t type) { m_type = type; }

		void SetDataType(uint32_t dataType) { m_dataType = dataType; }

		void SetDrawType(uint32_t drawType) { m_drawType = drawType; }

		uint32_t Type() const { return m_type; }

		uint32_t DataType() const { return m_dataType; }

		uint32_t DrawType() const { return m_drawType; }

		bool Uploaded() { return m_uploadedToGPU; }

		void SetUploaded(bool uploaded) { m_uploadedToGPU = uploaded; }

	protected:

		uint32_t m_type;
		uint32_t m_dataType;
		uint32_t m_drawType;

		bool m_uploadedToGPU = false;

		uint32_t m_id;
	};

	class IndexBuffer : public Buffer<uint32_t>, public GraphicsBuffer
	{
	public:
		IndexBuffer() {}

		IndexBuffer(uint32_t type, uint32_t data, uint32_t draw);

		IndexBuffer(vector<uint32_t>& indices, uint32_t type, uint32_t data, uint32_t draw);

		~IndexBuffer();
	};

	class VertexBuffer : public Buffer<float>, public GraphicsBuffer
	{
	public:
		VertexBuffer() {}

		VertexBuffer(uint32_t type, uint32_t data, uint32_t draw);

		VertexBuffer(vector<float>& vertices, uint32_t type, uint32_t data, uint32_t draw);

		~VertexBuffer();

		uint32_t SizeInBytes();
	};	

	pair<glm::vec3, glm::vec3> findMaxMinPair(VertexBuffer&  vertices);
}

#endif