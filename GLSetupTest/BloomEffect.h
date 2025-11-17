#ifndef BLOOM_EFFECT_H
#define BLOOM_EFFECT_H

#include <string>
#include <glm/glm.hpp>
#include <vector>

namespace JLEngine
{
	class ResourceLoader;
	class RenderTarget;
	class ShaderProgram;

	struct BloomMip
	{
		glm::ivec2 size{};
		RenderTarget* texture = nullptr;
	};

	class BloomEffect
	{
	public:
		BloomEffect();
		~BloomEffect();

		void Initialise(ResourceLoader* loader, const std::string& assetPath, int initialWidth, int initialHeight);
		void OnResize(int newWidth, int newHeight);

		RenderTarget* Render(RenderTarget* hdrSceneTexture, int iterations = 6);

		float threshold = 0.5f;
		float intensity = 0.1f;
		float knee = 0.2f;
		float scatter = 0.3f;
		bool enabled = true;

	private:

		void CreateMipChain(int width, int height);
		void DestroyMipChain();

		std::string m_assetPath;
		ResourceLoader* m_loader;

		ShaderProgram* m_prefilterShader = nullptr;
		ShaderProgram* m_downsampleShader = nullptr;
		ShaderProgram* m_upsampleShader = nullptr;

		std::vector<BloomMip> m_mipChain;
		int m_maxIterations = 6;
	};
}

#endif