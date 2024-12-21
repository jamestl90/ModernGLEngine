#ifndef CUBEMAP_BAKER_H
#define CUBEMAP_BAKER_H

#include "Types.h"
#include "AssetLoader.h"

#include <string>

namespace JLEngine
{
    class Texture;
	class ShaderProgram;
	class ImageData;

	class CubemapBaker
	{
	public:
		CubemapBaker(const std::string& assetPath, AssetLoader* assetLoader);
		~CubemapBaker();

		uint32_t HDRtoCubemap(ImageData& imgData, int cubeMapSize, bool genMipmaps);
		uint32_t GenerateIrradianceCubemap(uint32_t hdrCubeMapID, unsigned int irradianceMapSize);
		uint32_t GeneratePrefilteredEnvMap(uint32_t hdrCubeMapID, unsigned int prefEnvMapSize, int numSamples = 1024);

		void CleanupInternals();

	private: 
		bool SaveCubemapToFile(uint32_t cubemapID, int cubeMapSize, const std::string& outputFilePath);
		uint32_t CreateEmptyCubemap(int cubeMapSize);
		void EnsureHDRtoCubemapShadersLoaded(const std::string& fullPath);
		void EnsureIrradianceShadersLoaded(const std::string& fullPath);
		void EnsureDownsampleCubemapShadersLoaded(const std::string& fullPath);
		void EnsurePrefilteredCubemapShadersLoaded(const std::string& fullPath);

		std::string m_assetPath;

		AssetLoader* m_assetLoader;
		ShaderProgram* m_hdrToCubemapShader;
		ShaderProgram* m_downsampleCubemapShader;
		ShaderProgram* m_irradianceShader;
		ShaderProgram* m_prefilteredCubemapShader;
	};
}

#endif