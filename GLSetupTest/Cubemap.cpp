#include "Cubemap.h" 
#include "GraphicsAPI.h"

#include <stdexcept>

namespace JLEngine
{
    Cubemap::Cubemap(const std::string& name, GraphicsAPI* graphics)
        : GraphicsResource(name, graphics)
    {
        
    }

    Cubemap::~Cubemap()
    {

    }

    void Cubemap::InitFromData(std::array<ImageData, 6>&& faceData)
    {
        m_cubemapFaceData = std::move(faceData);
    }

    void Cubemap::SetParams(const TexParams& params)
    {
        m_texParams = params;

        // set defaults for others
        if (m_texParams.wrapS == 0) m_texParams.wrapS = GL_REPEAT;
        if (m_texParams.wrapT == 0) m_texParams.wrapT = GL_REPEAT;
        if (m_texParams.wrapR == 0) m_texParams.wrapR = GL_REPEAT;
        if (m_texParams.minFilter == 0) m_texParams.minFilter = GL_LINEAR;
        if (m_texParams.magFilter == 0) m_texParams.magFilter = GL_LINEAR;
    }
}