#ifndef INDIRECT_DRAW_BUFFER_H
#define INDIRECT_DRAW_BUFFER_H

#include "VertexBuffers.h"

namespace JLEngine
{
	struct alignas(16) PerDrawData
	{
		glm::mat4 modelMatrix;			// 64 bytes
		uint32_t materialID;			// 4 bytes
	};

	struct DrawIndirectCommand
	{
		uint32_t count;
		uint32_t instanceCount;
		uint32_t firstIndex;
		uint32_t baseVertex;
		uint32_t baseInstance;
	};

	class IndirectDrawBuffer : public Buffer<DrawIndirectCommand>, public GraphicsBuffer
	{
	public:
		IndirectDrawBuffer()
			: GraphicsBuffer(GL_DRAW_INDIRECT_BUFFER, GL_UNSIGNED_INT, GL_STATIC_DRAW) {}

		IndirectDrawBuffer(uint32_t draw)
			: GraphicsBuffer(GL_DRAW_INDIRECT_BUFFER, GL_UNSIGNED_INT, draw) {}

		IndirectDrawBuffer(uint32_t draw, const std::vector<DrawIndirectCommand>& commands)
			: GraphicsBuffer(GL_DRAW_INDIRECT_BUFFER, GL_UNSIGNED_INT, draw)
		{
			m_buffer = commands;
		}

		void AddDrawCommand(const DrawIndirectCommand& command)
		{
			m_buffer.push_back(command);
			SetDirty(true); // Mark buffer as needing an update
		}

		void RemoveDrawCommand(size_t index)
		{
			if (index < m_buffer.size())
			{
				m_buffer.erase(m_buffer.begin() + index);
				SetDirty(true);
			}
		}

		void UpdateDrawCommand(size_t index, const DrawIndirectCommand& command)
		{
			if (index < m_buffer.size())
			{
				m_buffer[index] = command;
				SetDirty(true);
			}
		}

		void ClearCommands()
		{
			m_buffer.clear();
			SetDirty(true);
		}

		const std::vector<DrawIndirectCommand>& GetDrawCommands() const { return m_buffer; }
		const DrawIndirectCommand& GetDrawCommands(size_t i) const { return m_buffer[i]; }
	};
}

#endif