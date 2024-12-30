#ifndef GPU_RESOURCE_H
#define GPU_RESOURCE_H

#include "Resource.h"

#include <string>

namespace JLEngine
{
	class GPUResource : public Resource
	{
	public:
		GPUResource(const std::string& name) : Resource(name), m_gpuID(0) {}

		uint32_t GetGPUID() const { return m_gpuID; }
		void SetGPUID(uint32_t id) { m_gpuID = id; }

	protected:
		uint32_t m_gpuID;
	};
}

#endif