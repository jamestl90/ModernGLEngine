#include "ShaderStorageBuffer.h"
#include <glad/glad.h> // Assuming GL functions are accessed through glad

namespace JLEngine 
{
    ShaderStorageBuffer::ShaderStorageBuffer(const std::string& name, size_t size, Graphics* graphics)
        : Resource(name), m_size(size), m_graphics(graphics), m_ssboID(0) {}

    ShaderStorageBuffer::~ShaderStorageBuffer() 
    {
        if (m_ssboID) {
            glDeleteBuffers(1, &m_ssboID);
        }
    }

    void ShaderStorageBuffer::Initialize() 
    {
        glGenBuffers(1, &m_ssboID);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_ssboID);
        glBufferData(GL_SHADER_STORAGE_BUFFER, m_size, nullptr, GL_DYNAMIC_DRAW);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }

    void ShaderStorageBuffer::UpdateData(const void* data, size_t size) 
    {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_ssboID);
        glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, size, data);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }

    void ShaderStorageBuffer::Bind(uint32_t bindingPoint) 
    {
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, bindingPoint, m_ssboID);
    }

} // namespace JLEngine
