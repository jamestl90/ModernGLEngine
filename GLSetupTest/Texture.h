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

        static TexParams EmptyParams()
        {
            TexParams params;
            params.dataType = 0;
            params.format = 0;
            params.internalFormat = 0;
            params.magFilter = 0;
            params.minFilter = 0;
            params.mipmapEnabled = 0;
            params.textureType = 0;
            params.wrapR = 0;
            params.wrapS = 0;
            params.wrapT = 0;
            return params;
        }

        static TexParams OverwriteParams(TexParams target, TexParams overwrite)
        {
            if (overwrite.dataType > 0) target.dataType = overwrite.dataType;
            if (overwrite.format > 0) target.format = overwrite.format;
            if (overwrite.internalFormat > 0) target.internalFormat = overwrite.internalFormat;
            if (overwrite.magFilter > 0) target.magFilter = overwrite.magFilter;
            if (overwrite.minFilter > 0) target.minFilter = overwrite.minFilter;
            if (overwrite.textureType > 0) target.textureType = overwrite.textureType;
            if (overwrite.wrapR > 0) target.wrapR = overwrite.wrapR;
            if (overwrite.wrapS > 0) target.wrapS = overwrite.wrapS;
            if (overwrite.wrapT > 0) target.wrapT = overwrite.wrapT;
            return target;
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
            texParams.internalFormat = hdr ? GL_RGB16F : GL_RGB8;
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