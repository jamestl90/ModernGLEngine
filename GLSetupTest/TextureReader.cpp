#include <glad/glad.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <iostream>
#include "TextureReader.h"

namespace JLEngine
{
    TextureReader::TextureReader() {}

    TextureReader::~TextureReader() {}

    ImageData TextureReader::LoadTexture(const std::string& filePath, int numCompsDesired) 
    {
        ImageData imageData;

        if (filePath.ends_with(".hdr")) 
        {
            imageData.isHDR = true;
            stbi_set_flip_vertically_on_load(true);
            float* hdrPixels = stbi_loadf(filePath.c_str(), &imageData.width, &imageData.height, &imageData.channels, numCompsDesired);
            if (!hdrPixels) 
            {
                std::cerr << "Failed to load HDR texture: " << filePath << std::endl;
                return imageData;
            }
            imageData.hdrData.assign(hdrPixels, hdrPixels + (imageData.width * imageData.height * imageData.channels));
            stbi_image_free(hdrPixels);
        }
        else 
        {
            unsigned char* pixels = stbi_load(filePath.c_str(), &imageData.width, &imageData.height, &imageData.channels, numCompsDesired);
            if (!pixels) 
            {
                std::cerr << "Failed to load texture: " << filePath << std::endl;
                return imageData;
            }
            imageData.data.assign(pixels, pixels + (imageData.width * imageData.height * imageData.channels));
            stbi_image_free(pixels);
        }

        return imageData;
    }

    std::array<ImageData, 6> TextureReader::LoadCubeMapHDR(const std::string& folderPath, const std::array<std::string, 6>& fileNames)
    {
        if (fileNames.size() != 6) 
        {
            throw std::invalid_argument("Cube map requires exactly 6 textures.");
        }

        auto finalAssetPath = [&](size_t index) { return folderPath + fileNames[index]; };

        std::array<ImageData, 6> cubeFaceData;

        try 
        {
            for (size_t i = 0; i < 6; ++i)
            {
                ImageData imgData;      
                float* stbData = stbi_loadf(finalAssetPath(i).c_str(), &imgData.width, &imgData.height, &imgData.channels, 0);                
                if (!stbData)
                {
                    std::cerr << "Failed to load HDR image: " << finalAssetPath(i) << std::endl;
                    throw std::runtime_error("Failed to load HDR image: " + finalAssetPath(i));
                }
                imgData.isHDR = true;
                imgData.hdrData.assign(stbData, stbData + (imgData.width * imgData.height * imgData.channels));
                stbi_image_free(stbData);
                cubeFaceData[i] = std::move(imgData);
            }
        }
        catch (const std::runtime_error&)
        {
            throw; // Rethrow the exception
        }

        return cubeFaceData;
    }

    float* TextureReader::StitchSky(const std::string& assetPath, std::initializer_list<const char*> fileNames, int width, int height, int channels)
    {
        int facePixelCount = width * height;
        auto totalPixelCount = facePixelCount * fileNames.size();
        float* finalData = new float[totalPixelCount * channels];

        int index = 0;
        for (auto fileName : fileNames)
        {
            auto finalAssetPath = assetPath + fileName;
            int tWidth, tHeight, tChannels;
            float* hdrData = stbi_loadf(finalAssetPath.c_str(), &tWidth, &tHeight, &tChannels, 0);

            if (!hdrData)
            {
                std::cerr << "Failed to load HDR image!" << std::endl;
                delete[] finalData;
                return nullptr;
            }
            if (width != tWidth || height != tHeight)
            {
                std::cerr << "Mismatched dimensions for file: " << finalAssetPath << std::endl;
                stbi_image_free(hdrData);
                delete[] finalData;
                return nullptr;
            }
            if (tChannels < channels)
            {
                std::cerr << "Texture has insufficient channels (expected at least 3): " << finalAssetPath << std::endl;
                stbi_image_free(hdrData);
                delete[] finalData;
                return nullptr;
            }

            std::memcpy(finalData + (index * facePixelCount * 3), hdrData, facePixelCount * channels * sizeof(float));
            index++;

            stbi_image_free(hdrData);
        }
        return finalData;
    }

    ImageData TextureReader::StitchSky(std::array<JLEngine::ImageData, 6>& files, int width, int height, int channels)
    {
        int facePixelCount = width * height;
        int totalPixelCount = facePixelCount * 6;

        std::vector<float> finalData(totalPixelCount * channels);

        for (int faceIndex = 0; faceIndex < 6; ++faceIndex)
        {
            const auto& imgData = files[faceIndex];

            if (imgData.hdrData.size() != facePixelCount * channels)
            {
                throw std::runtime_error("Input image dimensions do not match the expected width, height, and channels.");
            }

            int xOffset = faceIndex * width;

            for (int y = 0; y < height; ++y)
            {
                int srcOffset = y * width * channels;
                int dstOffset = (y * (width * 6) + xOffset) * channels;

                std::memcpy(
                    finalData.data() + dstOffset,
                    imgData.hdrData.data() + srcOffset,
                    width * channels * sizeof(float)
                );
            }
        }

        ImageData result;
        result.hdrData = std::move(finalData); 
        result.width = width * 6;              
        result.height = height;                
        result.channels = channels;
        result.isHDR = true;

        return result;
    }
}