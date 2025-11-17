#ifndef DIRECTIONAL_LIGHT_SHADOWMAP
#define DIRECTIONAL_LIGHT_SHADOWMAP

#include "Types.h"

#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace JLEngine
{
	class ShaderProgram;
	class GraphicsAPI;

	/*	
	*	https://learnopengl.com/Guest-Articles/2021/CSM
	*/
	class DirectionalLightShadowMap
	{
	public:
		DirectionalLightShadowMap(ShaderProgram* shader, 
								ShaderProgram* shaderSkinning, 
								int numCascades = 4,
								float maxShadowDistance = 50.0f);
		~DirectionalLightShadowMap();

		void DrawDebugUI();

		void Initialise(int shadowMapResolutionPerCascade = 4096);
		void UpdateCascades(const glm::mat4& cameraViewMatrix,
			const glm::mat4& cameraProjectionMatrix,
			const glm::vec3& lightDirection, float nearClip, float fovRad, float aspect);
		void BeginShadowMapPassForCascade(int cascadeIndex);
		void EndShadowMapPass();

		uint32_t GetShadowMapTextureArrayID() const { return m_cascadeShadowMapArrayTexture; }
		const std::vector<glm::mat4>& GetCascadeLightSpaceMatrices() const { return m_cascadeLightSpaceMatrices; }
		const std::vector<float>& GetCascadeFarSplitsViewSpace() const { return m_cascadeFarSplitsViewSpace; } // z values in view space
		int GetNumCascades() const { return m_numCascades; }

		ShaderProgram* GetShadowMapShader() { return m_shadowMapShader; }
		ShaderProgram* GetShadowMapSkinningShader() { return m_shadowMapSkinningShader; }
		float& GetDistance() { return m_maxShadowDistance; }
		float& GetBias() { return m_bias; } 
		int& GetPCFKernelSize() { return m_PCFKernelSize; }

		void SetModelMatrix(const glm::mat4& model);
		void SetMaxShadowDistance(float dist) { m_maxShadowDistance = dist; }
		void SetNumCascades(int num) { m_numCascades = num; /* Needs re-initialization of some resources */ }

	protected:

		void CalculateCascadeSplits(const glm::mat4& cameraProjectionMatrix, float nearVal);
		std::vector<glm::vec4> GetFrustumCornersWorldSpace(const glm::mat4& proj, const glm::mat4& view);
		glm::mat4 CalculateLightSpaceMatrixForCascade(const glm::vec3& lightDir, int cascadeIndex, const glm::mat4& cameraViewMatrix, float fovRad, float aspect);

		ShaderProgram* m_shadowMapShader;
		ShaderProgram* m_shadowMapSkinningShader;

		uint32_t m_cascadeShadowMapArrayTexture;
		uint32_t m_shadowFBO;

		int m_numCascades = 4;
		int m_shadowMapResolution = 2048;
		float m_maxShadowDistance = 50.0f;

		float m_bias = 0.00058f;
		int m_PCFKernelSize = 0;

		std::vector<glm::mat4> m_cascadeLightSpaceMatrices;
		std::vector<float> m_cascadeFarSplitsViewSpace;
	};
}

#endif