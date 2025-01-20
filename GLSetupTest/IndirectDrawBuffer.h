#ifndef INDIRECT_DRAW_BUFFER_H
#define INDIRECT_DRAW_BUFFER_H

#include "GPUBuffer.h"

#include <vector>
#include <glm/glm.hpp>

namespace JLEngine
{
	struct alignas(16) PerDrawData
	{
		glm::mat4 modelMatrix;			// 64 bytes
		uint32_t materialID;			// 4 bytes
	};

	struct alignas(16) SkinnedMeshPerDrawData
	{
		glm::mat4 modelMatrix;			// 64 bytes
		uint32_t materialID;			// 4 bytes
		uint32_t baseJointIndex;
	};

	struct DrawIndirectCommand
	{
		uint32_t count;
		uint32_t instanceCount;
		uint32_t firstIndex;
		uint32_t baseVertex; 
		uint32_t baseInstance;
	};

	class IndirectDrawBuffer 
	{
	public:
		IndirectDrawBuffer()
			: m_gpuBuffer(GL_DRAW_INDIRECT_BUFFER, GL_DYNAMIC_STORAGE_BIT) {}
		IndirectDrawBuffer(std::vector<DrawIndirectCommand>&& vertices, uint32_t usage = GL_DYNAMIC_STORAGE_BIT)
			: m_data(std::move(vertices)), m_gpuBuffer(GL_DRAW_INDIRECT_BUFFER, usage) {}
		~IndirectDrawBuffer() {}

		void AddDrawCommand(const DrawIndirectCommand& command)
		{
			m_data.push_back(command);
			m_gpuBuffer.ClearDirty();
		}

		void RemoveDrawCommand(size_t index)
		{
			if (index < m_data.size())
			{
				m_data.erase(m_data.begin() + index);
				m_gpuBuffer.ClearDirty();
			}
		}

		void UpdateDrawCommand(size_t index, const DrawIndirectCommand& command)
		{
			if (index < m_data.size())
			{
				m_data[index] = command;
				m_gpuBuffer.MarkDirty();
			}
		}

		void ClearCommands()
		{
			m_data.clear();
		}

		GPUBuffer& GetGPUBuffer() { return m_gpuBuffer; }
		const std::vector<DrawIndirectCommand>& GetDataImmutable() const { return m_data; }
		std::vector<DrawIndirectCommand>& GetDataMutable() { return m_data; }

	private:
		std::vector<DrawIndirectCommand> m_data;
		GPUBuffer m_gpuBuffer;
	};
}

#endif