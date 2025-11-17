#ifndef PHYSICALLY_BASED_SKY_H
#define PHYSICALLY_BASED_SKY_H

#include "AtmosphereParameters.h"
#include "UniformBuffer.h"
#include "VertexArrayObject.h"

namespace JLEngine
{
	class ShaderProgram;
	class ResourceLoader;

	class PhysicallyBasedSky
	{
	public:
		PhysicallyBasedSky(ResourceLoader* resourceLoader, const std::string& assetPath);
		~PhysicallyBasedSky();

		bool Initialise(const AtmosphereParams& params);
		
		void UpdateParams(const AtmosphereParams& params);

		void RenderSky(const glm::mat4& viewMatrix, const glm::mat4& projMatrix, const glm::vec3& eyePos);

		int GetTransmittanceLUT() const { return m_transmittanceLUT; }
		int GetAtmosphereUBO() { return m_paramsUBO.GetGPUBuffer().GetGPUID(); }

	private:
		void ComputeLUT();

		ResourceLoader* m_resourceLoader;

		AtmosphereParams m_params;
		UniformBuffer m_paramsUBO;

		VertexArrayObject m_triangleVAO;
		GLuint m_transmittanceLUT = 0;

		const glm::ivec2 TRANSMITTANCE_LUT_SIZE = glm::ivec2(256, 64);

		ShaderProgram* m_precomputeLUT;
		ShaderProgram* m_renderSky;

		bool m_initialised = false;
		bool m_lutsGenerated = false;

		std::string m_assetPath = "";
	};
}

#endif