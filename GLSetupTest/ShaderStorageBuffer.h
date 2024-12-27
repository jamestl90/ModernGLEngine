#ifndef SHADER_STORAGE_BUFFER_H
#define SHADER_STORAGE_BUFFER_H

#include "Resource.h"
#include "VertexBuffers.h"
#include <string>
#include "glm/glm.hpp"

namespace JLEngine 
{
    class GraphicsAPI;

    template <typename T>
    class ShaderStorageBuffer : public Buffer<T>, public GraphicsBuffer
    {
    public:
        ShaderStorageBuffer();
        ~ShaderStorageBuffer();

    };

    template <typename T>
    ShaderStorageBuffer<T>::ShaderStorageBuffer()
        : GraphicsBuffer(GL_SHADER_STORAGE_BUFFER, 0, GL_STATIC_DRAW)
    {

    }

    template <typename T>
    ShaderStorageBuffer<T>::~ShaderStorageBuffer()
    {

    }
} 

#endif 

