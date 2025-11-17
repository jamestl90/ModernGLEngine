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
			: GPUResource(""), m_type(0), m_usageFlags(0), m_bufferSize(0), m_isDirty(true), m_immutable(true) {}
		GPUBuffer(const std::string& name)
			: GPUResource(name), m_type(0), m_usageFlags(0), m_bufferSize(0), m_isDirty(true), m_immutable(true) {}
		GPUBuffer(uint32_t type, GLbitfield usageFlags)
			: GPUResource(""), m_type(type), m_usageFlags(usageFlags), m_bufferSize(0), m_isDirty(true), m_immutable(true) {}
		~GPUBuffer() { }

		const bool IsCreated()			const { return m_created; }
		const uint32_t GetType()		const { return m_type; }
		const size_t GetSizeInBytes()	const { return m_bufferSize; }
		const bool IsDirty()			const { return m_isDirty; }
		const bool IsImmutable()		const { return m_immutable; }

		void SetUsageFlags(GLbitfield flags)  { m_usageFlags = flags; }
		void SetCreated(bool created)		  { m_created = created; }
		void SetImmutable(bool immutable)	  { m_immutable = immutable; }
		void SetSizeInBytes(size_t size)	  { m_bufferSize = size; }
		void MarkDirty()					  { m_isDirty = true; }
		void ClearDirty()					  { m_isDirty = false; }

		GLbitfield GetUsageFlags() const { return m_usageFlags; }

	protected:

		uint32_t m_type;
		GLbitfield m_usageFlags;
		size_t m_bufferSize;
		bool m_isDirty = true;
		bool m_immutable = true;
		bool m_created = false;
	};
}

#endif