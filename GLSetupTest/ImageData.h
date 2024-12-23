#ifndef IMAGE_DATA_H
#define IMAGE_DATA_H

#include <vector>

#include <glad/glad.h>

namespace JLEngine
{
	class ImageData
	{
    public:
        int width          = 0;
        int height         = 0;
        int channels       = 0;

        uint32_t textureType    = GL_TEXTURE_2D;
        uint32_t internalFormat = GL_RGBA8;
        uint32_t format         = GL_RGBA;
        uint32_t dataType       = GL_UNSIGNED_BYTE;

        uint32_t minFilter      = GL_LINEAR;
        uint32_t maxFilter      = GL_LINEAR;
        uint32_t wrapS          = GL_CLAMP_TO_EDGE;
        uint32_t wrapT          = GL_CLAMP_TO_EDGE;
        uint32_t wrapR          = GL_CLAMP_TO_EDGE;

        bool isHDR;      // Whether the image is HDR
        std::vector<unsigned char> data; // Regular texture data
        std::vector<float> hdrData;     // HDR texture data

        ImageData() : width(0), height(0), channels(0), isHDR(false) {}

        ~ImageData() = default; // Use default destructor since vectors manage memory automatically

        // Move constructor
        ImageData(ImageData&& other) noexcept
            : width(other.width), height(other.height), channels(other.channels), isHDR(other.isHDR), 
            data(std::move(other.data)), hdrData(std::move(other.hdrData)) {}

        ImageData& operator=(ImageData&& other) noexcept 
        {
            if (this != &other) 
            { // Avoid self-assignment
                width = other.width;
                height = other.height;
                channels = other.channels;
                isHDR = other.isHDR;
                data = std::move(other.data);
                hdrData = std::move(other.hdrData);
                internalFormat = other.internalFormat;
                format = other.format;
                dataType = other.dataType;
                minFilter = other.minFilter;
                maxFilter = other.maxFilter;
                wrapS = other.wrapS;
                wrapT = other.wrapT;
                wrapR = other.wrapR;
                textureType = other.textureType;
            }
            return *this;
        }

        // Disable copy
        ImageData(const ImageData&) = delete;
        ImageData& operator=(const ImageData&) = delete;

        // Utility method to check if data is valid
        bool IsValid() const 
        {
            return (!isHDR && !data.empty()) || (isHDR && !hdrData.empty());
        }
	};
}

#endif