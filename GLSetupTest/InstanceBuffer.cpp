#include "InstanceBuffer.h"

namespace JLEngine
{
	InstanceBuffer::~InstanceBuffer() { UnloadFromGPU(); }

	void InstanceBuffer::UploadToGPU(JLEngine::Graphics* graphics, const std::vector<glm::mat4>& instanceTransforms)
	{
		m_graphics = graphics;

		m_graphics->CreateInstanceBuffer(*this, instanceTransforms);
	}

	void InstanceBuffer::UpdateData(std::vector<glm::mat4>& instanceTransforms)
	{
		m_graphics->BindBuffer(GL_ARRAY_BUFFER, m_instanceId);
		if (instanceTransforms.size() > m_instanceCount)
		{
			// Reallocate buffer if size increases
			m_graphics->SetBufferData(GL_ARRAY_BUFFER, instanceTransforms.size() * sizeof(glm::mat4), instanceTransforms.data(),
				m_static ? GL_STATIC_DRAW : GL_DYNAMIC_DRAW);
		}
		else
		{
			m_graphics->SetBufferSubData(GL_ARRAY_BUFFER, 0, instanceTransforms.size() * sizeof(glm::mat4), instanceTransforms.data());
		}
		m_instanceCount = (uint32)instanceTransforms.size();
	}

	void InstanceBuffer::UnloadFromGPU()
	{
		if (m_graphics)
		{
			m_graphics->DisposeInstanceBuffer(*this);
		}
	}
}