#ifndef BAKER_H
#define BAKER_H

#include "Types.h"
#include "AssetLoader.h"

#include <string>

namespace JLEngine
{
    class Texture;
	class ShaderProgram;
	class ImageData;

	class Baker
	{
	public:
		Baker(const std::string& assetPath, AssetLoader* assetLoader);
		~Baker();

		uint32 HDRtoCubemap(ImageData imgData, int cubeMapSize);
		uint32 GenerateIrradianceCubemap(uint32 cubeMapId, int irradianceMapSize);

		void CleanupInternals();

	private: 
		void RenderCubemapFromHDR(int cubeMapSize, uint32 cubemapID, const ImageData& hdrTexture);
		void SetupCaptureFramebuffer(int cubeMapSize, uint32 cubemapID);
		bool SaveCubemapToFile(uint32 cubemapID, int cubeMapSize, const std::string& outputFilePath);
		uint32 CreateEmptyCubemap(int cubeMapSize);
		void EnsureHDRtoCubemapShadersLoaded(const std::string& fullPath);
		void EnsureIrradianceShadersLoaded(const std::string& fullPath);

		std::string m_assetPath;

		AssetLoader* m_assetLoader;
		ShaderProgram* m_hdrToCubemapShader;
		ShaderProgram* m_irradianceShader;
	};
}

#endif