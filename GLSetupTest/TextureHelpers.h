#ifndef TEXTURE_HELPERS_H
#define TEXTURE_HELPERS_H

#include <string>
#include <initializer_list>

namespace JLEngine
{
	class TextureHelpers
	{
	public:
		TextureHelpers();

		static float* StitchSky(const std::string& assetPath, std::initializer_list<const char*> fileNames, int width, int height, int channels);
		static bool WriteHDR(const std::string& assetPath, const std::string& fileName, int width, int height, int channels, float* data);

	protected:

	};
}

#endif