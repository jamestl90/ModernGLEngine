#ifndef GPU_BUFFER_H
#define GPU_BUFFER_H

#include <glad/glad.h>

#include "GPUResource.h"

namespace JLEngine
{
	class GPUBuffer : public GPUResource
	{
	public:
		GPUBuffer()
			: GPUResource(""), m_type(0), m_usageFlags(0), m_bufferSize(0), m_isDirty(true) {}
		GPUBuffer(const std::string& name)
			: GPUResource(name), m_type(0), m_usageFlags(0), m_bufferSize(0), m_isDirty(true) {}
		GPUBuffer(uint32_t type, GLbitfield usageFlags)
			: GPUResource(""), m_type(type), m_usageFlags(usageFlags), m_bufferSize(0), m_isDirty(true) {}
		~GPUBuffer() {}

		uint32_t GetType() const { return m_type; }
		size_t GetSize() const { return m_bufferSize; }
		bool IsDirty() const { return m_isDirty; }

		void SetSize(size_t size) { m_bufferSize = size; }
		void MarkDirty() { m_isDirty = true; }
		void ClearDirty() { m_isDirty = false; }

		GLbitfield GetUsageFlags() const { return m_usageFlags; }

	protected:

		uint32_t m_type;
		GLbitfield m_usageFlags;
		size_t m_bufferSize;
		bool m_isDirty = true;
	};
}

#endif