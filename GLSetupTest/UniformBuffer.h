#ifndef UNIFORM_BUFFER_H
#define UNIFORM_BUFFER_H

#include "GPUBuffer.h"

#include <string>

namespace JLEngine
{
	class UniformBuffer
	{
	public:
		UniformBuffer(GLbitfield usageFlags = GL_DYNAMIC_STORAGE_BIT)
			: m_gpuBuffer(GL_UNIFORM_BUFFER, usageFlags), m_bindingPoint(0) {}

		uint32_t GetBindingPoint() const { return m_bindingPoint; }
		GPUBuffer& GetGPUBuffer() { return m_gpuBuffer; }

	protected:
		GPUBuffer m_gpuBuffer;
		uint32_t m_bindingPoint;
	};
}

#endif