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

		uint32 GetShadowMapID() { return m_shadowMap; }
		float& GetBias() { return m_bias; }
		
		void SetModelMatrix(glm::mat4& model);

	protected:

		ShaderProgram* m_shadowMapShader;
		uint32 m_shadowMap;
		uint32 m_shadowFBO;
		int m_shadowMapSize = 4096;
		float m_bias = 0.003f;

		Graphics* m_graphics;
	};
}

#endif