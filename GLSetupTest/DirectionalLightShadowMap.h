#ifndef DIRECTIONAL_LIGHT_SHADOWMAP
#define DIRECTIONAL_LIGHT_SHADOWMAP

#include "Types.h"
#include "DeferredRenderer.h"

namespace JLEngine
{
	class ShaderProgram;
	class GraphicsAPI;

	class DirectionalLightShadowMap
	{
	public:
		DirectionalLightShadowMap(GraphicsAPI* graphics, ShaderProgram* shader, ShaderProgram* shaderSkinning);

		void Initialise();
		void ShadowMapPassSetup(const glm::mat4& lightSpaceMatrix);

		uint32_t GetShadowMapID() { return m_shadowMap; }
		float& GetBias() { return m_bias; }
		float& GetSize() { return m_size; }
		float& GetShadowDistance() { return m_dlShadowDistance; }
		int& GetPCFKernelSize() { return m_PCFKernelSize; }
		ShaderProgram* GetShadowMapShader() { return m_shadowMapShader; }
		ShaderProgram* GetShadowMapSkinningShader() { return m_shadowMapSkinningShader; }
		
		void SetModelMatrix(glm::mat4& model);
		void SetSize(float size) { m_size = size; }
		void SetShadowDistance(float dist) { m_dlShadowDistance = dist; }
		void SetPCFKernelSize(int size) { m_PCFKernelSize = size; }

	protected:

		ShaderProgram* m_shadowMapShader;
		ShaderProgram* m_shadowMapSkinningShader;
		uint32_t m_shadowMap;
		uint32_t m_shadowFBO;
		int m_shadowMapSize = 4096;
		float m_bias = 0.0009f;
		float m_size = 15.0f;
		float m_dlShadowDistance;
		int m_PCFKernelSize;

		GraphicsAPI* m_graphics;
	};
}

#endif