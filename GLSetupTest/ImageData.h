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
            }
            return *this;
        }

        // Disable copy
        ImageData(const ImageData&) = delete;
        ImageData& operator=(const ImageData&) = delete;
        
        static ImageData CreateDefaultImageData(int width, int height, GLenum format, const std::vector<unsigned char>& defaultData)
        {
            JLEngine::ImageData imageData;
            imageData.width = width;
            imageData.height = height;
            imageData.channels = (format == GL_RGBA) ? 4 : (format == GL_RGB) ? 3 : (format == GL_RG) ? 2 : 1; // Determine channels from format
            imageData.isHDR = false;

            // Fill the data buffer with default values
            size_t pixelCount = width * height * imageData.channels;
            imageData.data.resize(pixelCount);
            std::copy(defaultData.begin(), defaultData.end(), imageData.data.begin());

            return imageData;
        }

        // Utility method to check if data is valid
        bool IsValid() const 
        {
            return (!isHDR && !data.empty()) || (isHDR && !hdrData.empty());
        }
	};
}

#endif