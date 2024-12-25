#ifndef TEXTURE_READER_H
#define TEXTURE_READER_H

#include <string>
#include <array>

#include "Types.h"
#include "ImageData.h"

namespace JLEngine
{
    class TextureReader
    {
    public:
        TextureReader();
        ~TextureReader();

        static ImageData LoadTexture(const std::string& filePath, int numCompsDesired = 0);
        static std::array<ImageData, 6> LoadCubeMapHDR(const std::string& folderPath, const std::array<std::string, 6>& fileNames);
        static float* StitchSky(const std::string& assetPath, std::initializer_list<const char*> fileNames, int width, int height, int channels);
        static ImageData StitchSky(std::array<JLEngine::ImageData, 6>& files, int width, int height, int channels);

    private:

    };
}

#endif