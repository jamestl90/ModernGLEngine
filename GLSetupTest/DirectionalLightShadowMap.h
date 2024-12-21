#ifndef DIRECTIONAL_LIGHT_SHADOWMAP
#define DIRECTIONAL_LIGHT_SHADOWMAP

#include "Types.h"
#include "DeferredRenderer.h"

namespace JLEngine
{
	class ShaderProgram;
	class Graphics;

	class DirectionalLightShadowMap
	{
	public:
		DirectionalLightShadowMap(Graphics* graphics, ShaderProgram* shader);

		void Initialise();
		void ShadowMapPassSetup(const glm::mat4& lightSpaceMatrix);

		uint32_t GetShadowMapID() { return m_shadowMap; }
		float& GetBias() { return m_bias; }
		float& GetSize() { return m_size; }
		float& GetShadowDistance() { return m_dlShadowDistance; }
		int& GetPCFKernelSize() { return m_PCFKernelSize; }
		
		void SetModelMatrix(glm::mat4& model);
		void SetSize(float size) { m_size = size; }
		void SetShadowDistance(float dist) { m_dlShadowDistance = dist; }
		void SetPCFKernelSize(int size) { m_PCFKernelSize = size; }

	protected:

		ShaderProgram* m_shadowMapShader;
		uint32_t m_shadowMap;
		uint32_t m_shadowFBO;
		int m_shadowMapSize = 4096;
		float m_bias = 0.0009f;
		float m_size = 25.0f;
		float m_dlShadowDistance;
		int m_PCFKernelSize;

		Graphics* m_graphics;
	};
}

#endif