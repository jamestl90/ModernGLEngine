#include "TextureWriter.h"
#include <stb_image_write.h>

namespace JLEngine
{
    bool TextureWriter::WriteTexture(const std::string& filePath, const ImageData& imageData) 
    {
        if (imageData.isHDR) {
            return WriteHDR(filePath, imageData);
        }
        else {
            return WriteRegular(filePath, imageData);
        }
    }

    bool TextureWriter::WriteHDR(const std::string& filePath, const ImageData& imageData) 
    {
        if (stbi_write_hdr(filePath.c_str(), imageData.width, imageData.height, imageData.channels, imageData.hdrData.data())) {
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
}