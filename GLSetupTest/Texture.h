#ifndef TEXTURE_H
#define TEXTURE_H

#include "ImageData.h"
#include "GraphicsResource.h"

#include <array>
#include <string>

namespace JLEngine
{
	class GraphicsAPI;

    struct TexParams
    {
        bool mipmapEnabled =        false;
        bool immutable =            false;
        uint32_t wrapS =            GL_CLAMP_TO_EDGE;
        uint32_t wrapT =            GL_CLAMP_TO_EDGE;
        uint32_t wrapR =            GL_CLAMP_TO_EDGE;
        uint32_t minFilter =        GL_LINEAR;
        uint32_t magFilter =        GL_LINEAR;
        uint32_t textureType =      GL_TEXTURE_2D;
        uint32_t internalFormat =   GL_RGBA;
        uint32_t format =           GL_RGBA;
        uint32_t dataType =         GL_UNSIGNED_BYTE;
    };

    class Texture : public GraphicsResource
    {
    public:
        Texture(const std::string& name, GraphicsAPI* graphics);
        ~Texture();

        const ImageData& GetImageData() const { return m_imageData; }
        ImageData& GetMutableImageData() { return m_imageData; }        

        void InitFromData(ImageData&& imageData);

        const TexParams& GetParams() const { return m_texParams; }
        void SetParams(const TexParams& params);

        void SetFormat(uint32_t dataType, uint32_t internalFormat, uint32_t format);

        static TexParams RGBATexParams()
        {
            TexParams texParams;
            texParams.internalFormat = GL_RGBA;
            texParams.format = GL_RGBA;
            return texParams;
        }

        static TexParams RGBTexParams()
        {
            TexParams texParams;
            texParams.internalFormat = GL_RGB;
            texParams.format = GL_RGB;
            return texParams;
        }

    private:
        GraphicsAPI* m_graphics = nullptr;  

        ImageData m_imageData;
        TexParams m_texParams;
    };
}

#endif