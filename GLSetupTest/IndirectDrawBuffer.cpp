#include "IndirectDrawBuffer.h"
#include "Graphics.h"
#include "GraphicsAPI.h"

namespace JLEngine
{
	void IndirectDrawBuffer::SyncToGPU()
	{
		if (m_dirty)
		{
			Graphics::API()->BindBuffer(GL_DRAW_INDIRECT_BUFFER, this->GetId());
			if (m_buffer.empty())
			{
				Graphics::API()->SetBufferData(GL_DRAW_INDIRECT_BUFFER, 0, nullptr, DrawType());
			}
			else
			{
				Graphics::API()->SetBufferData(GL_DRAW_INDIRECT_BUFFER,
					m_buffer.size() * sizeof(DrawIndirectCommand), m_buffer.data(), DrawType());
			}
			m_dirty = false;
		}
	}
}