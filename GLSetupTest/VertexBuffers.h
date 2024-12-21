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

		GraphicsBuffer(int type, int data, int drawType);

		void SetId(uint32_t& id) { m_id = id; m_uploadedToGPU = true; }

		uint32_t GetId() const { return m_id; }

		void SetType(int type) { m_type = type; }

		void SetDataType(int dataType) { m_dataType = dataType; }

		void SetDrawType(int drawType) { m_drawType = drawType; }

		int Type() const { return m_type; }

		int DataType() const { return m_dataType; }

		int DrawType() const { return m_drawType; }

		bool Uploaded() { return m_uploadedToGPU; }

		void SetUploaded(bool uploaded) { m_uploadedToGPU = uploaded; }

	protected:

		int m_type;
		int m_dataType;
		int m_drawType;

		bool m_uploadedToGPU = false;

		uint32_t m_id;
	};

	class IndexBuffer : public Buffer<uint32_t>, public GraphicsBuffer
	{
	public:
		IndexBuffer() {}

		IndexBuffer(int type, int data, int draw);

		IndexBuffer(vector<uint32_t>& indices, int type, int data, int draw);

		~IndexBuffer();
	};

	class VertexBuffer : public Buffer<float>, public GraphicsBuffer
	{
	public:
		VertexBuffer() : m_stride(0), m_vaoId(0), m_key(0) {}

		VertexBuffer(int type, int data, int draw);

		VertexBuffer(vector<float>& vertices, int type, int data, int draw);

		~VertexBuffer();

		void Add(glm::vec3& val);

		void AddAttribute(const VertexAttribute& attrib);

		void CalcStride();

		const std::set<VertexAttribute>& GetAttributes() const;

		uint32_t GetStride();

		uint32_t SizeInBytes();

		void SetVAO(uint32_t id) { m_vaoId = id; }
		uint32_t GetVAO() { return m_vaoId; }

		VertexAttribKey GetAttribKey() { return m_key; }
		void SetVertexAttribKey(VertexAttribKey key) { m_key = key; }

	private:

		VertexAttribKey m_key;

		uint32_t m_vaoId;

		std::set<VertexAttribute> m_attributes;

		uint32_t m_stride;
	};

	pair<glm::vec3, glm::vec3> findMaxMinPair(VertexBuffer&  vertices);
}

#endif