#include "TextureHelpers.h"
#include <stb_image.h>
#include <stb_image_write.h>
#include <iostream>

#include "Types.h"

namespace JLEngine
{
    TextureHelpers::TextureHelpers()
    {
    }

    float* TextureHelpers::StitchSky(const std::string& assetPath, std::initializer_list<const char*> fileNames, int width, int height, int channels)
    {
        int facePixelCount = width * height;
        int totalPixelCount = facePixelCount * fileNames.size();
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

    bool TextureHelpers::WriteHDR(const std::string& assetPath, const std::string& fileName, int width, int height, int channels, float* data)
    {
        auto outputFile = assetPath + fileName;
        if (!stbi_write_hdr(outputFile.c_str(), width, height, channels, data))
        {
            std::cerr << "Failed to write HDR image to file: " << outputFile << std::endl;
            return false;
        }
        std::cout << "Successfully saved HDR texture to: " << outputFile << std::endl;
        return true;
    }
}