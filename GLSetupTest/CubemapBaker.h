#ifndef CUBEMAP_BAKER_H
#define CUBEMAP_BAKER_H

#include "Types.h"
#include "ResourceLoader.h"

#include <string>

namespace JLEngine
{
    class Texture;
	class ShaderProgram;
	class ImageData;

	class CubemapBaker
	{
	public:
		CubemapBaker(const std::string& assetPath, ResourceLoader* assetLoader);
		~CubemapBaker();

		uint32_t HDRtoCubemap(ImageData& imgData, int cubeMapSize, bool genMipmaps, float compressionThresh = 3.0f, float maxValue = 10000);
		uint32_t GenerateIrradianceCubemap(uint32_t hdrCubeMapID, unsigned int irradianceMapSize);
		uint32_t GeneratePrefilteredEnvMap(uint32_t hdrCubeMapID, unsigned int prefEnvMapSize, int numSamples = 1024);
		uint32_t GenerateBRDFLUT(int lutSize = 512, int numSamples = 1024);

		void CleanupInternals();

	private: 
		uint32_t CreateEmptyCubemap(int cubeMapSize);
		void EnsureHDRtoCubemapShadersLoaded(const std::string& fullPath);
		void EnsureIrradianceShadersLoaded(const std::string& fullPath);
		void EnsurePrefilteredCubemapShadersLoaded(const std::string& fullPath);
		void EnsureBRDFLutShaderLoaded(const std::string& fullPath);

		std::string m_assetPath;

		ResourceLoader* m_assetLoader;
		ShaderProgram* m_hdrToCubemapShader;
		ShaderProgram* m_irradianceShader;
		ShaderProgram* m_prefilteredCubemapShader;
		ShaderProgram* m_brdfLutShader;
	};
}

#endif