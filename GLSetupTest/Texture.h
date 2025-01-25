#ifndef TEXTURE_H
#define TEXTURE_H

#include "ImageData.h"
#include "GPUResource.h"

#include <array>
#include <string>
#include <stdexcept>

namespace JLEngine
{
    struct TexParams
    {
        bool mipmapEnabled =        true;
        uint32_t wrapS =            GL_REPEAT;
        uint32_t wrapT =            GL_REPEAT;
        uint32_t wrapR =            GL_REPEAT;
        uint32_t minFilter =        GL_LINEAR;
        uint32_t magFilter =        GL_LINEAR;
        uint32_t textureType =      GL_TEXTURE_2D;
        uint32_t internalFormat =   GL_RGB8;
        uint32_t format =           GL_RGB;
        uint32_t dataType =         GL_UNSIGNED_BYTE;
    };

    class Texture : public GPUResource
    {
    public:
        Texture() : GPUResource("") {}
        Texture(const std::string& name);
        ~Texture();

        const ImageData& GetImageData() const { return m_imageData; }
        ImageData& GetMutableImageData() { return m_imageData; }        

        void InitFromData(ImageData&& imageData);

        // this will not move the data but probably hard copy it
        // only use this when you want to keep the original ImageData
        // or if imagedata has no actual data in it
        void InitFromData(ImageData& imageData);

        const TexParams& GetParams() const { return m_texParams; }
        void SetParams(const TexParams& params);

        void SetFormat(uint32_t dataType, uint32_t internalFormat, uint32_t format);

        uint64_t Bindless() const { return m_bindlessHandle; }
        void SetBindlessHandle(uint64_t handle) { m_bindlessHandle = handle; }

        static TexParams DefaultParams(int channels, bool hdr)
        {
            TexParams texParams;
            switch (channels)
            {
            case 1: // Single channel (e.g., grayscale)
                texParams.format = GL_RED;
                texParams.internalFormat = hdr ? GL_R32F : GL_R8;
                break;
            case 2: // Two channels (e.g., RG)
                texParams.format = GL_RG;
                texParams.internalFormat = hdr ? GL_RG32F : GL_RG8;
                break;
            case 3: // Three channels (e.g., RGB)
                texParams.format = GL_RGB;
                texParams.internalFormat = hdr ? GL_RGB32F : GL_RGB8;
                break;
            case 4: // Four channels (e.g., RGBA)
                texParams.format = GL_RGBA;
                texParams.internalFormat = hdr ? GL_RGBA32F : GL_RGBA8;
                break;
            default:
                throw std::invalid_argument("Invalid number of channels for texture");
            }
            texParams.dataType = hdr ? GL_FLOAT : GL_UNSIGNED_BYTE;
            return texParams;
        }

        static TexParams RGBATexParams(bool hdr = false)
        {
            TexParams texParams;
            texParams.internalFormat = hdr ? GL_RGBA16F : GL_RGBA8;
            texParams.format = GL_RGBA;
            return texParams;
        }

        static TexParams RGBTexParams(bool hdr = false)
        {
            TexParams texParams;
            texParams.internalFormat = hdr ? GL_RGB16F : GL_RGB8;;
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