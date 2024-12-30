#ifndef TEXTURE_H
#define TEXTURE_H

#include "ImageData.h"
#include "GPUResource.h"

#include <array>
#include <string>

namespace JLEngine
{
    struct TexParams
    {
        bool mipmapEnabled =        true;
        bool immutable =            false;
        uint32_t wrapS =            GL_REPEAT;
        uint32_t wrapT =            GL_REPEAT;
        uint32_t wrapR =            GL_REPEAT;
        uint32_t minFilter =        GL_LINEAR;
        uint32_t magFilter =        GL_LINEAR;
        uint32_t textureType =      GL_TEXTURE_2D;
        uint32_t internalFormat =   GL_RGBA;
        uint32_t format =           GL_RGBA;
        uint32_t dataType =         GL_UNSIGNED_BYTE;
    };

    class Texture : public GPUResource
    {
    public:
        Texture(const std::string& name);
        ~Texture();

        const ImageData& GetImageData() const { return m_imageData; }
        ImageData& GetMutableImageData() { return m_imageData; }        

        void InitFromData(ImageData&& imageData);

        const TexParams& GetParams() const { return m_texParams; }
        void SetParams(const TexParams& params);

        void SetFormat(uint32_t dataType, uint32_t internalFormat, uint32_t format);

        uint64_t Bindless() const { return m_bindlessHandle; }
        void SetBindlessHandle(uint64_t handle) { m_bindlessHandle = handle; }

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
        uint64_t m_bindlessHandle = 0;

        ImageData m_imageData;
        TexParams m_texParams;
    };
}

#endif