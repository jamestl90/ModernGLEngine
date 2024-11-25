#include <glad/glad.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <iostream>
#include "TextureReader.h"

namespace JLEngine
{
    TextureReader::TextureReader() {}

    TextureReader::~TextureReader() {}

    bool TextureReader::ReadTexture(const std::string& texture, std::vector<unsigned char>& outData,
        int& outWidth, int& outHeight, int& outChannels)
    {
        // Use stb_image to load the image
        unsigned char* data = stbi_load(texture.c_str(), &outWidth, &outHeight, &outChannels, 0);

        if (data == nullptr) // Check if loading failed
        {
            std::cerr << "Failed to load texture: " << texture
                << " (" << stbi_failure_reason() << ")" << std::endl;
            return false;
        }

        // Copy the data into the provided container
        size_t dataSize = outWidth * outHeight * outChannels;
        outData.assign(data, data + dataSize);

        // Free stb_image's allocated memory
        stbi_image_free(data);

        return true;
    }
}