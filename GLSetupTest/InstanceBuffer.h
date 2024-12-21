#ifndef INSTANCE_BUFFER_H
#define INSTANCE_BUFFER_H

#include "Types.h"

#include <vector>

#include <glm/glm.hpp>

namespace JLEngine
{
	class Graphics;

	class InstanceBuffer
	{
	public:
		InstanceBuffer(bool isStatic) : m_static(isStatic) {}
		~InstanceBuffer();

		void UploadToGPU(Graphics* graphics, const std::vector<glm::mat4>& instanceTransforms);

		void UpdateData(std::vector<glm::mat4>& instanceTransforms);

		void UnloadFromGPU();

		void SetGPUID(uint32_t id) { m_instanceId = id; m_uploadedToGPU = true; }
		void SetInstanceCount(uint32_t instanceCount) { m_instanceCount = instanceCount; }

		uint32_t GetGPUID() const { return m_instanceId; }
		uint32_t GetInstanceCount() const { return m_instanceCount; }
		bool IsStatic() const { return m_static; }
		bool Uploaded() const { return m_uploadedToGPU; }

		static constexpr uint32_t INSTANCE_MATRIX_LOCATION = 3;
	protected:
		bool m_uploadedToGPU = false;
		Graphics* m_graphics = nullptr;
		bool m_static = false;
		uint32_t m_instanceId = 0;
		uint32_t m_instanceCount = 0;
	};
}

#endif