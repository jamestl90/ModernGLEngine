#ifndef POST_PROCESSING_H
#define POST_PROCESSING_H

#include <glm/glm.hpp>
#include <vector>
#include <string>

namespace JLEngine
{
	class ResourceLoader;
	class BloomEffect;
	class RenderTarget;
	class ShaderProgram;

	class PostProcessing
	{
	public:
		PostProcessing(ResourceLoader* loader, const std::string& assetPath);
		~PostProcessing();

		void Initialise(int width, int height);

		void Render(
			RenderTarget* lightOutput,
			RenderTarget* finalOutputTarget, 
			int width, int height,
			glm::vec3& eyePos, 
			glm::mat4& viewMat,
			glm::mat4& projMat);
		void OnResize(int newWidth, int newHeight);
		BloomEffect* GetBloom() { return m_bloom; }

		void DrawDebugUI();

		float tonemapExposure = 1.0f;

	private:

		int m_iterations = 6;

		ResourceLoader* m_loader;
		std::string m_assetPath;

		BloomEffect* m_bloom;
		ShaderProgram* m_combineShader = nullptr;
	};
}

#endif