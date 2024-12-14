#ifndef BAKING_H
#define BAKING_H

#include "Types.h"
#include "AssetLoader.h"

#include <string>

namespace JLEngine
{
    class Texture;
	class ShaderProgram;

	class Baking
	{
	public:
		Baking(AssetLoader* assetLoader);

        Texture* HDRtoCubemap(const std::string& name, const std::string& assetPath, int cubeMapSize = 2048);

		void CleanupInternals();

	private: 

		void LoadHDRtoCubemapShaders(const std::string& fullPath);

		AssetLoader* m_assetLoader;
		ShaderProgram* m_hdrToCubemapShader;
	};
}

#endif