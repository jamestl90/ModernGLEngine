#ifndef SHADER_STORAGE_BUFFER_H
#define SHADER_STORAGE_BUFFER_H

#include "GPUBuffer.h"

#include <glm/glm.hpp>

#include <string>
#include <vector>

namespace JLEngine 
{
    template <typename T>
    class ShaderStorageBuffer
    {
    public:
        ShaderStorageBuffer(uint32_t usageFlags = GL_DYNAMIC_STORAGE_BIT)
            : m_gpuBuffer(GL_SHADER_STORAGE_BUFFER, usageFlags) {}

        ~ShaderStorageBuffer() = default;

        // Access GPUBuffer
        GPUBuffer& GetGPUBuffer() { return m_gpuBuffer; }
        const GPUBuffer& GetGPUBuffer() const { return m_gpuBuffer; }

        // Access Data
        const std::vector<T>& GetDataImmutable() const { return m_data; }
        std::vector<T>& GetDataMutable() { return m_data; }

        void Set(std::vector<T>&& data) { m_data = data; }

        void AddData(const T& element)
        {
            m_data.push_back(element);
            m_gpuBuffer.MarkDirty();
        }

        size_t SizeInBytes()
        {
            return m_data.size() * sizeof(T);
        }

    private:
        std::vector<T> m_data;
        GPUBuffer m_gpuBuffer;
    };
} 

#endif 

