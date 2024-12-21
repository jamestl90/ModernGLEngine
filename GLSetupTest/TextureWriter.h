#ifndef TEXTURE_WRITER_H
#define TEXTURE_WRITER_H

#include <string>
#include "ImageData.h"
#include <iostream>
#include <array>

namespace JLEngine
{
	class TextureWriter
	{
	public:
        static bool WriteTexture(const std::string& filePath, const ImageData& imageData);

		static void SaveCubemapAsCrossLayout(std::array<ImageData, 6>& imgData, const std::string& outputPath);

    private:
        static bool WriteHDR(const std::string& filePath, const ImageData& imageData);

        static bool WriteRegular(const std::string& filePath, const ImageData& imageData);
	};
}

#endif