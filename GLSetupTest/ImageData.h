#ifndef IMAGE_DATA_H
#define IMAGE_DATA_H

#include <vector>

namespace JLEngine
{
	class ImageData
	{
    public:
        int width;
        int height;
        int channels;    // Number of channels (1 for grayscale, 3 for RGB, etc.)

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
            if (this != &other) { // Avoid self-assignment
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

        // Utility method to check if data is valid
        bool IsValid() const 
        {
            return (!isHDR && !data.empty()) || (isHDR && !hdrData.empty());
        }
	};
}

#endif