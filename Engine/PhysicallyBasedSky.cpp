#include "PhysicallyBasedSky.h"
#include "ResourceLoader.h"

namespace JLEngine
{
	PhysicallyBasedSky::PhysicallyBasedSky(ResourceLoader* resourceLoader, const std::string& assetPath)
		: m_resourceLoader(resourceLoader), m_assetPath(assetPath), m_renderSky(nullptr), m_params({}), m_precomputeLUT(nullptr)
	{
	}

	PhysicallyBasedSky::~PhysicallyBasedSky()
	{
		if (m_triangleVAO.GetGPUID() > 0) Graphics::API()->DeleteVertexArray(m_triangleVAO.GetGPUID());
		Graphics::API()->DeleteTexture(1, &m_transmittanceLUT);
		Graphics::DisposeGPUBuffer(&m_paramsUBO.GetGPUBuffer());
	}

	bool PhysicallyBasedSky::Initialise(const AtmosphereParams& params)
	{
		m_params = params;

		auto finalAssetPath = m_assetPath + "Core/Shaders/Sky/";
		m_renderSky = m_resourceLoader->CreateShaderFromFile("PhysicallyBasedSky", 
															 "physicallybasedsky_vert.glsl",
															 "physicallybasedsky_frag.glsl", 
															 finalAssetPath).get();
		m_precomputeLUT = m_resourceLoader->CreateComputeFromFile("TransmittanceLUT",
																  "transmittance_lut.compute",
																  finalAssetPath).get();

		// init ubo size in bytes 
		m_paramsUBO.GetGPUBuffer().SetSizeInBytes(sizeof(AtmosphereParams));
		Graphics::CreateGPUBuffer(m_paramsUBO.GetGPUBuffer());
		Graphics::API()->DebugLabelObject(GL_BUFFER, m_paramsUBO.GetGPUBuffer().GetGPUID(), "AtmosphereParams");

		// upload data
		Graphics::UploadToGPUBuffer(m_paramsUBO.GetGPUBuffer(), m_params, 0);

		// still need a vao even if there are no vertices defined 
		m_triangleVAO.SetGPUID(Graphics::API()->CreateVertexArray());

		ComputeLUT();

		m_initialised = true;
		return m_initialised;
	}

	void PhysicallyBasedSky::UpdateParams(const AtmosphereParams& params)
	{
		m_params = params;
		if (m_initialised)
			Graphics::UploadToGPUBuffer(m_paramsUBO.GetGPUBuffer(), m_params, 0);

		ComputeLUT();
	}

	void PhysicallyBasedSky::RenderSky(const glm::mat4& viewMatrix, const glm::mat4& projMatrix, const glm::vec3& eyePos)
	{
		if (!m_initialised) return;

		bool usingDepth = Graphics::API()->IsEnabled(GL_DEPTH_TEST);
		int prevDepthFunc = Graphics::API()->GetInteger(GL_DEPTH_FUNC);
		bool prevDepthMask = Graphics::API()->GetBoolean(GL_DEPTH_WRITEMASK);

		Graphics::API()->Enable(GL_DEPTH_TEST);
		Graphics::API()->SetDepthFunc(GL_LEQUAL);
		Graphics::API()->SetDepthMask(GL_FALSE);

		Graphics::API()->BindShader(m_renderSky->GetProgramId());
		Graphics::BindGPUBuffer(m_paramsUBO.GetGPUBuffer(), 0);

		glm::mat4 invView = glm::inverse(viewMatrix);
		glm::mat4 invProj = glm::inverse(projMatrix);

		m_renderSky->SetUniform("invViewMatrix", invView);
		m_renderSky->SetUniform("invProjMatrix", invProj);

		float bottomRadiusMeters = m_params.bottomRadiusKM * 1000.0f;
		glm::vec3 planetCenterCameworld = glm::vec3(0.0f, -bottomRadiusMeters, 0.0f);
		glm::vec3 eyePosKM = eyePos * 0.001f;
		glm::vec3 planetCenterKM = planetCenterCameworld * 0.001f;
		glm::vec3 pbSkyCamPos = eyePosKM - planetCenterKM;
		m_renderSky->SetUniform("cameraPos_world_km", pbSkyCamPos);

		Graphics::API()->BindTextureUnit(0, m_transmittanceLUT);

		Graphics::API()->BindVertexArray(m_triangleVAO.GetGPUID());
		Graphics::API()->DrawArrayBuffer(GL_TRIANGLES, 0, 3);
		Graphics::API()->BindVertexArray(0);

		if (!usingDepth) Graphics::API()->Disable(GL_DEPTH_TEST);
		Graphics::API()->SetDepthFunc(prevDepthFunc);
		Graphics::API()->SetDepthMask(prevDepthMask);

		Graphics::API()->BindBufferBase(GL_UNIFORM_BUFFER, 0, 0);
		Graphics::API()->BindShader(0); // Unbind program
	}

	void PhysicallyBasedSky::ComputeLUT()
	{
		if (m_transmittanceLUT == 0)
		{
			Graphics::API()->CreateTextures(GL_TEXTURE_2D, 1, &m_transmittanceLUT);
			Graphics::API()->TextureStorage2D(m_transmittanceLUT, 1, GL_RGBA16F,
				TRANSMITTANCE_LUT_SIZE.x, TRANSMITTANCE_LUT_SIZE.y);
			Graphics::API()->TextureParameter(m_transmittanceLUT, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			Graphics::API()->TextureParameter(m_transmittanceLUT, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			Graphics::API()->TextureParameter(m_transmittanceLUT, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			Graphics::API()->TextureParameter(m_transmittanceLUT, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		}

		Graphics::API()->BindShader(m_precomputeLUT->GetProgramId());
		Graphics::BindGPUBuffer(m_paramsUBO.GetGPUBuffer(), 0);
		Graphics::API()->BindImageTexture(0, m_transmittanceLUT, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);

		GLuint numGroupsX = (TRANSMITTANCE_LUT_SIZE.x + 7) / 8;
		GLuint numGroupsY = (TRANSMITTANCE_LUT_SIZE.y + 7) / 8;

		Graphics::API()->DispatchCompute(numGroupsX, numGroupsY, 1);

		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);

		Graphics::API()->BindImageTexture(0, 0, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
		Graphics::API()->BindShader(0);
		Graphics::API()->BindBufferBase(GL_UNIFORM_BUFFER, 0, 0);
	}
}