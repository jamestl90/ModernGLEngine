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
		void ShadowMapPassSetup(const glm::mat4& lightView, const glm::mat4& lightProjection);

		uint32 GetShadowMapID() { return m_shadowMap; }

	protected:

		ShaderProgram* m_shadowMapShader;
		uint32 m_shadowMap;
		uint32 m_shadowFBO;
		int m_shadowMapSize = 2048;

		Graphics* m_graphics;
	};
}

#endif