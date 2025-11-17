#include "TextureWriter.h"
#include <stb_image_write.h>

namespace JLEngine
{
    bool TextureWriter::WriteTexture(const std::string& filePath, const ImageData& imageData) 
    {
        if (imageData.isHDR) 
        {
            return WriteHDR(filePath, imageData);
        }
        else 
        {
            return WriteRegular(filePath, imageData);
        }
    }

    bool TextureWriter::WriteHDR(const std::string& filePath, const ImageData& imageData) 
    {
        size_t expectedSize = imageData.width * imageData.height * imageData.channels;
        if (imageData.hdrData.size() != expectedSize)
        {
            std::cerr << "Error: HDR data size mismatch for " << filePath
                << " (expected " << expectedSize << ", got " << imageData.hdrData.size() << ")." << std::endl;
            return false;
        }

        if (stbi_write_hdr(filePath.c_str(), imageData.width, imageData.height, imageData.channels, imageData.hdrData.data())) 
        {
            std::cout << "Successfully wrote HDR texture: " << filePath << std::endl;
            return true;
        }
        std::cerr << "Failed to write HDR texture: " << filePath << std::endl;
        return false;
    }

    bool TextureWriter::WriteRegular(const std::string& filePath, const ImageData& imageData) 
    {
        // Determine format based on file extension
        if (filePath.ends_with(".png")) 
        {
            if (stbi_write_png(filePath.c_str(), imageData.width, imageData.height, imageData.channels, imageData.data.data(), 0)) {
                std::cout << "Successfully wrote PNG texture: " << filePath << std::endl;
                return true;
            }
        }
        else if (filePath.ends_with(".jpg") || filePath.ends_with(".jpeg")) 
        {
            if (stbi_write_jpg(filePath.c_str(), imageData.width, imageData.height, imageData.channels, imageData.data.data(), 100)) {
                std::cout << "Successfully wrote JPG texture: " << filePath << std::endl;
                return true;
            }
        }
        else if (filePath.ends_with(".bmp")) 
        {
            if (stbi_write_bmp(filePath.c_str(), imageData.width, imageData.height, imageData.channels, imageData.data.data())) {
                std::cout << "Successfully wrote BMP texture: " << filePath << std::endl;
                return true;
            }
        }

        std::cerr << "Failed to write regular texture: " << filePath << std::endl;
        return false;
    }


    void TextureWriter::SaveCubemapAsCrossLayout(std::array<ImageData, 6>& imgData, const std::string& outputPath)
    {
        // Get face dimensions
        int faceWidth = imgData[0].width;
        int faceHeight = imgData[0].height;
        int channels = imgData[0].channels;
        bool isHDR = imgData[0].isHDR;

        // Combined image dimensions (cross layout)
        int totalWidth = faceWidth * 4;
        int totalHeight = faceHeight * 3;

        // Allocate memory for combined image
        size_t pixelSize = isHDR ? sizeof(float) : sizeof(unsigned char);
        std::vector<unsigned char> imageData(totalWidth * totalHeight * channels * pixelSize, 0);

        auto writeFace = [&](int offsetX, int offsetY, const std::vector<unsigned char>& faceData) {
            for (int y = 0; y < faceHeight; ++y)
            {
                for (int x = 0; x < faceWidth; ++x)
                {
                    for (int c = 0; c < channels; ++c)
                    {
                        size_t srcIndex = (y * faceWidth + x) * channels + c;
                        size_t dstIndex = ((offsetY + y) * totalWidth + (offsetX + x)) * channels + c;
                        imageData[dstIndex] = faceData[srcIndex];
                    }
                }
            }
            };

        // Map cubemap faces to the cross layout
        writeFace(faceWidth, 0, imgData[2].data); // +Y
        writeFace(0, faceHeight, imgData[0].data); // -X
        writeFace(faceWidth, faceHeight, imgData[4].data); // -Z
        writeFace(2 * faceWidth, faceHeight, imgData[1].data); // +X
        writeFace(3 * faceWidth, faceHeight, imgData[5].data); // +Z
        writeFace(faceWidth, 2 * faceHeight, imgData[3].data); // -Y

        // Save the combined image
        if (isHDR)
        {
            stbi_write_hdr(outputPath.c_str(), totalWidth, totalHeight, channels, reinterpret_cast<float*>(imageData.data()));
        }
        else
        {
            stbi_write_png(outputPath.c_str(), totalWidth, totalHeight, channels, imageData.data(), totalWidth * channels);
        }

        std::cout << "Saved cubemap as cross layout to " << outputPath << std::endl;
    }
}