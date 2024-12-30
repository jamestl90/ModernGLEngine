#ifndef VERTEX_BUFFERS_H
#define VERTEX_BUFFERS_H

#include "VertexStructures.h"
#include "Resource.h"
#include "Graphics.h"
#include "GraphicsAPI.h"
#include "GPUBuffer.h"

#include <glm/glm.hpp>
#include <set>
#include <vector>
#include <memory>

using std::pair;
using std::vector;

namespace JLEngine
{
	class IndexBuffer
	{
	public:
		IndexBuffer() 
			: m_gpuBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_DYNAMIC_STORAGE_BIT) {}		
		IndexBuffer(const std::string& name)
			:  m_gpuBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_DYNAMIC_STORAGE_BIT) {}
		IndexBuffer(vector<uint32_t>&& indices, uint32_t usage = GL_DYNAMIC_STORAGE_BIT)
			: m_data(std::move(indices)), m_gpuBuffer(GL_ELEMENT_ARRAY_BUFFER, usage) {}
		~IndexBuffer() {}

		GPUBuffer& GetGPUBuffer() { return m_gpuBuffer; }
		const std::vector<uint32_t>& GetDataImmutable() const { return m_data; }
		std::vector<uint32_t>& GetDataMutable() { return m_data; }

		void Set(std::vector<uint32_t>&& indices) { m_data = indices; }

		uint32_t SizeInBytes() const
		{
			return sizeof(uint32_t) * static_cast<uint32_t>(m_data.size());
		}

	private:
		std::vector<uint32_t> m_data;
		GPUBuffer m_gpuBuffer;
	};

	class VertexBuffer
	{
	public:
		VertexBuffer() 
			: m_gpuBuffer(GL_ARRAY_BUFFER, GL_DYNAMIC_STORAGE_BIT) {}
		VertexBuffer(const std::string& name)
			: m_gpuBuffer(GL_ARRAY_BUFFER, GL_DYNAMIC_STORAGE_BIT) {}
		VertexBuffer(vector<float>&& vertices, uint32_t usage = GL_DYNAMIC_STORAGE_BIT)
			: m_data(std::move(vertices)), m_gpuBuffer(GL_ARRAY_BUFFER, usage) {}
		~VertexBuffer() {}

		GPUBuffer& GetGPUBuffer() { return m_gpuBuffer; }
		const std::vector<float>& GetDataImmutable() const { return m_data; }
		std::vector<float>& GetDataMutable() { return m_data; }

		void Set(std::vector<float>&& indices) { m_data = indices; }

		uint32_t SizeInBytes() const
		{
			return sizeof(float) * static_cast<uint32_t>(m_data.size());
		}

	private:
		std::vector<float> m_data;
		GPUBuffer m_gpuBuffer;
	};	
}

#endif