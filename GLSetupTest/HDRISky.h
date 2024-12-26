#ifndef HDRI_SKY_H
#define HDRI_SKY_H

#include <glm/glm.hpp>

#include "VertexArrayObject.h"
#include "ImageData.h"
#include "CubemapBaker.h"

namespace JLEngine
{
	struct HdriSkyInitParams
	{
		std::string fileName;
		
		int irradianceMapSize = 32;
		int prefilteredMapSize = 128;
		int prefilteredSamples = 2048;
	};

	class ResourceLoader;

	class HDRISky
	{
	public:
		HDRISky(ResourceLoader* resourceLoader); 
		~HDRISky();

		void Initialise(const std::string& assetPath, const HdriSkyInitParams& initParams);
		void Render(const glm::mat4& viewMatrix, const glm::mat4& projMatrix, int debugTexId = 0);

		const uint32_t& GetSkyGPUID() const { return m_hdriSky; }
		const uint32_t& GetIrradianceGPUID() const { return m_irradianceMap; }
		const uint32_t& GetPrefilteredGPUID() const { return m_prefilteredMap; }
		const uint32_t& GetBRDFLutGPUID() const { return m_brdfLUTMap; }

		const ShaderProgram* GetSkyShader() const { return m_skyShader; }
		const VertexArrayObject& GetSkyboxVAO() const { return m_skyboxVAO; }

	protected:

		uint32_t m_hdriSky			= 0;
		uint32_t m_irradianceMap	= 0;
		uint32_t m_prefilteredMap	= 0;
		uint32_t m_brdfLUTMap		= 0;

        VertexArrayObject m_skyboxVAO;
		ShaderProgram* m_skyShader;
		ResourceLoader* m_resourceLoader;

		ImageData m_hdriSkyImageData;
	};
}

#endif