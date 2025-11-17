#ifndef SKY_PROBE_H
#define SKY_PROBE_H

#include "Graphics.h"
#include "VertexArrayObject.h"
#include "Geometry.h"
#include "ResourceLoader.h"
#include "ShaderProgram.h"

#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>

namespace JLEngine
{
	class SkyProbe
	{
	public:
		GLuint environmentMapTex = 0;
		GLuint prefilteredTex = 0;
		GLuint captureFBO = 0;

		SkyProbe(ResourceLoader* loader, const std::string& assetPath) : m_loader(loader)
		{
			m_bakeCubemapShader = m_loader->CreateShaderFromFile("CubemapProbeShader",
																 "bake_sky_cubemap_vert.glsl",
																 "bake_sky_cubemap_frag.glsl",
																 assetPath).get();

			m_prefilterShader = m_loader->CreateShaderFromFile("PrefilterProbeShader",
															   "prefilter_cubemap_vert.glsl", 
															   "prefilter_cubemap_frag.glsl", 
															   assetPath).get();

			Geometry::CreateBox(m_vao);
			Graphics::CreateVertexArray(&m_vao);

			Graphics::API()->CreateFrameBuffer(1, &captureFBO);

			// create environment map tex
			Graphics::API()->CreateTextures(GL_TEXTURE_CUBE_MAP, 1, &environmentMapTex);
			Graphics::API()->TextureStorage2D(environmentMapTex, 1, GL_RGB16F, cubemapSize, cubemapSize);
			Graphics::API()->TextureParameter(environmentMapTex, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			Graphics::API()->TextureParameter(environmentMapTex, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			Graphics::API()->TextureParameter(environmentMapTex, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
			Graphics::API()->TextureParameter(environmentMapTex, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			Graphics::API()->TextureParameter(environmentMapTex, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			// create prefilter map (for specular reflections with roughness)
			m_maxMipLevels = static_cast<int>(std::floor(std::log2(prefilteredSize))) + 1;
			m_maxMipLevels = std::clamp(m_maxMipLevels, 1, 5); 
			Graphics::API()->CreateTextures(GL_TEXTURE_CUBE_MAP, 1, &prefilteredTex);
			Graphics::API()->TextureStorage2D(prefilteredTex, m_maxMipLevels, GL_RGB16F, prefilteredSize, prefilteredSize);
			Graphics::API()->TextureParameter(prefilteredTex, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			Graphics::API()->TextureParameter(prefilteredTex, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			Graphics::API()->TextureParameter(prefilteredTex, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
			Graphics::API()->TextureParameter(prefilteredTex, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); 
			Graphics::API()->TextureParameter(prefilteredTex, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		}

		~SkyProbe()
		{
			if (captureFBO > 0)			Graphics::API()->DeleteFrameBuffer(1, &captureFBO);
			if (environmentMapTex > 0)	Graphics::API()->DeleteTexture(1, &environmentMapTex);
			if (prefilteredTex > 0)		Graphics::API()->DeleteTexture(1, &prefilteredTex);
		}

		void ProcessBake()
		{
			if (!m_isBakeInProgress) return;

			Graphics::API()->BindFrameBuffer(captureFBO);
			Graphics::API()->BindVertexArray(m_vao.GetGPUID());

			// --- BAKE ENVIRONMENT MAP --- 
			if (m_currentBakeStage == CurrentBakeStage::BAKING_ENV_MAP)
			{
				Graphics::API()->BindShader(m_bakeCubemapShader->GetProgramId());
				Graphics::API()->BindBufferBase(GL_UNIFORM_BUFFER, 0, m_atmosphereParamsUBO);
				Graphics::API()->BindTextureUnit(0, m_transmittanceLUT);
				m_bakeCubemapShader->SetUniform("u_Projection", m_captureProjection);

				float bottomRadiusMeters = m_params.bottomRadiusKM * 1000.0f;
				glm::vec3 planetCenterCameworld = glm::vec3(0.0f, -bottomRadiusMeters, 0.0f);
				glm::vec3 eyePosKM = m_pos * 0.001f;
				glm::vec3 planetCenterKM = planetCenterCameworld * 0.001f;
				glm::vec3 pbSkyCamPos = eyePosKM - planetCenterKM;
				m_bakeCubemapShader->SetUniform("cameraPos_world_km", pbSkyCamPos);

				Graphics::API()->SetViewport(0, 0, cubemapSize, cubemapSize);
				m_bakeCubemapShader->SetUniform("u_View", m_captureViews[m_envMapFaceToBake]);
				Graphics::API()->NamedFramebufferTextureLayer(captureFBO, GL_COLOR_ATTACHMENT0, environmentMapTex, 0, m_envMapFaceToBake);
				Graphics::API()->Clear(GL_COLOR_BUFFER_BIT);
				Graphics::API()->DrawArrayBuffer(GL_TRIANGLES, 0, 36);

				m_envMapFaceToBake++;
				if (m_envMapFaceToBake >= 6) 
				{
					m_currentBakeStage = CurrentBakeStage::BAKING_PREFILTER_MAP;
					m_envMapFaceToBake = 0; // Reset for a potential next full bake cycle (optional here or in RestartBake)
				}
			}
			// --- BAKE PREFILTERED MAP ---
			else if (m_currentBakeStage == CurrentBakeStage::BAKING_PREFILTER_MAP)
			{
				Graphics::API()->BindShader(m_prefilterShader->GetProgramId());
				m_prefilterShader->SetUniform("u_Projection", m_captureProjection);
				m_prefilterShader->SetUniformi("u_NumSamples", m_numSamples);
				Graphics::API()->BindTextureUnit(0, environmentMapTex);

				int mip = m_prefilterMipToBake;
				int face = m_prefilterFaceToBake;

				uint32_t mipDim = prefilteredSize >> mip;
				if (mipDim == 0) mipDim = 1;
				Graphics::API()->SetViewport(0, 0, mipDim, mipDim);
				float roughness = (m_maxMipLevels > 1) ? (static_cast<float>(mip) / static_cast<float>(m_maxMipLevels - 1)) : 0.0f;
				m_prefilterShader->SetUniformf("u_Roughness", roughness);
				m_prefilterShader->SetUniform("u_View", m_captureViews[face]);
				Graphics::API()->NamedFramebufferTextureLayer(captureFBO,
					GL_COLOR_ATTACHMENT0,
					prefilteredTex,
					mip, face);
				Graphics::API()->Clear(GL_COLOR_BUFFER_BIT);
				Graphics::API()->BindVertexArray(m_vao.GetGPUID());
				Graphics::API()->DrawArrayBuffer(GL_TRIANGLES, 0, 36);

				m_prefilterFaceToBake++;
				if (m_prefilterFaceToBake >= 6)
				{
					m_prefilterFaceToBake = 0;
					m_prefilterMipToBake++;
					if (m_prefilterMipToBake >= m_maxMipLevels)
					{
						m_currentBakeStage = CurrentBakeStage::IDLE;
						m_isBakeInProgress = false;
					}
				}
			}
			Graphics::API()->BindFrameBuffer(0);
			Graphics::API()->BindVertexArray(0);
			Graphics::API()->BindBufferBase(GL_UNIFORM_BUFFER, 0, 0);
		}

		void DebugDrawSkybox(ShaderProgram* skyboxShader, const glm::mat4& viewMatrix, const glm::mat4& projMatrix)
		{
			GLint previousDepthFunc;
			glGetIntegerv(GL_DEPTH_FUNC, &previousDepthFunc);

			Graphics::API()->SetDepthFunc(GL_LEQUAL);
			Graphics::API()->BindShader(skyboxShader->GetProgramId());

			skyboxShader->SetUniform("u_Projection", projMatrix);
			skyboxShader->SetUniform("u_View", glm::mat4(glm::mat3(viewMatrix)));
			Graphics::API()->SetActiveTexture(0);
			Graphics::API()->BindTexture(GL_TEXTURE_CUBE_MAP, environmentMapTex);
			skyboxShader->SetUniformi("u_Skybox", 0);

			Graphics::API()->BindVertexArray(m_vao.GetGPUID());
			Graphics::API()->DrawArrayBuffer(GL_TRIANGLES, 0, 36);

			Graphics::API()->BindVertexArray(0);
			Graphics::API()->SetDepthFunc(previousDepthFunc);
		}

		void StartNewBake(glm::vec3 pos, AtmosphereParams& params, GLuint atmosphereParamsUBO, GLuint transmittanceLUT)
		{
			m_isBakeInProgress = true;
			m_currentBakeStage = CurrentBakeStage::BAKING_ENV_MAP;
			m_envMapFaceToBake = 0;
			m_prefilterMipToBake = 0;
			m_prefilterFaceToBake = 0;
			m_params = params;
			m_atmosphereParamsUBO = atmosphereParamsUBO;
			m_transmittanceLUT = transmittanceLUT;
			m_pos = pos;
		}

	private:

		enum class CurrentBakeStage
		{
			IDLE,
			BAKING_ENV_MAP,
			BAKING_PREFILTER_MAP
		};

		CurrentBakeStage m_currentBakeStage = CurrentBakeStage::BAKING_ENV_MAP;

		const glm::mat4 m_captureViews[6] =
		{
			glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
			glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
			glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
			glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
			glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
			glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
		};
		const glm::mat4 m_captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);

		glm::vec3 m_pos;
		AtmosphereParams m_params;
		GLuint m_atmosphereParamsUBO;
		GLuint m_transmittanceLUT;

		VertexArrayObject m_vao;
		ResourceLoader* m_loader;
		ShaderProgram* m_bakeCubemapShader;
		ShaderProgram* m_prefilterShader;
		int m_maxMipLevels = 0;
		int m_numSamples = 1024;
		uint32_t cubemapSize = 256;
		uint32_t prefilteredSize = 128;

		int m_envMapFaceToBake = 0;         
		int m_prefilterMipToBake = 0;       
		int m_prefilterFaceToBake = 0;
		bool m_isBakeInProgress = true;
	};
}

#endif