#ifndef SHADER_STORAGE_BUFFER_H
#define SHADER_STORAGE_BUFFER_H

#include "Resource.h"
#include <string>

namespace JLEngine 
{
    class GraphicsAPI;

    class ShaderStorageBuffer : public Resource 
    {
    public:
        ShaderStorageBuffer(const std::string& name, size_t size, GraphicsAPI* graphics);
        ~ShaderStorageBuffer();

        void Initialize();  // Allocate GPU memory
        void UpdateData(const void* data, size_t size);  // Update SSBO data
        void Bind(uint32_t bindingPoint);  // Bind SSBO to a specific binding point

    private:
        uint32_t m_ssboID;   // OpenGL ID for the SSBO
        size_t m_size;     // Size of the buffer
        GraphicsAPI* m_graphics;
    };

} 

#endif 

