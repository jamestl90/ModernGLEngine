#include "VertexArrayObject.h"
#include "Graphics.h"

namespace JLEngine
{
    VertexArrayObject::VertexArrayObject()
        : GraphicsResource("", Graphics::API()), m_key(0), m_stride(0)
    {
        
    }
}