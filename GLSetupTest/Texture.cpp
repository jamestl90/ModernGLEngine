#include "Texture.h"
#include "GraphicsAPI.h"

namespace JLEngine
{
    Texture::Texture(const std::string& name)
        : GPUResource(name)
    {
    }

    Texture::~Texture()
    {
    }

    void Texture::InitFromData(ImageData&& imageData)
    {
        if (!imageData.IsValid())
        {
            std::cerr << "Texture::InitFromImageData: Invalid ImageData provided." << std::endl;
            return;
        }

        // Move ImageData into the internal member
        m_imageData = std::move(imageData);
    }

    void Texture::SetParams(const TexParams& params) 
    {
        m_texParams = params; 

        // set defaults for others
        if (m_texParams.wrapS == 0) m_texParams.wrapS = GL_REPEAT;
        if (m_texParams.wrapT == 0) m_texParams.wrapT = GL_REPEAT;
        if (m_texParams.wrapR == 0) m_texParams.wrapR = GL_REPEAT;
        if (m_texParams.minFilter == 0) m_texParams.minFilter = GL_LINEAR;
        if (m_texParams.magFilter == 0) m_texParams.magFilter = GL_LINEAR;
    }

    void Texture::SetFormat(uint32_t dataType, uint32_t internalFormat, uint32_t format)
    {
        m_texParams.dataType = dataType;
        m_texParams.internalFormat = internalFormat;
        m_texParams.format = format;
    }
}