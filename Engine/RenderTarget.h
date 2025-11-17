#ifndef RENDER_TARGET_H
#define RENDER_TARGET_H

#include "Types.h"
#include "Texture.h"
#include "GPUResource.h"

#include <vector>

namespace JLEngine
{
	struct RTParams
	{
		GLuint internalFormat = GL_RGBA8;			// Internal format (e.g., GL_RGBA8, GL_RGB32F)
		GLuint minFilter =		GL_LINEAR;			// Minification filter
		GLuint magFilter =		GL_LINEAR;			// Magnification filter
		GLuint wrapS =			GL_CLAMP_TO_EDGE;	// Wrap mode for S
		GLuint wrapT =			GL_CLAMP_TO_EDGE;	// Wrap mode for T
	};

	enum class DepthType 
	{
		None,
		Renderbuffer,
		Texture,
		DepthStencil
	};

	class RenderTarget : public GPUResource
	{
	public:
		RenderTarget(const string& name);
		RenderTarget() : GPUResource(""), m_dbo(0), m_numSources(1),
			m_height(0), m_width(0), m_depthType(DepthType::None), m_useWindowSize(false) {}
		~RenderTarget();

		void SetDepthId(uint32_t id) { m_dbo = id; }
		void SetTexId(uint32_t index, uint32_t id) { m_textures[index] = id; }

		const uint32_t GetSamples() const { return m_numSamples; }
		const uint32_t GetDepthBufferId() const { return m_dbo; }
		uint32_t GetTexId(int index) { return m_textures[index]; }
		const uint32_t GetNumTextures() const { return m_numSources; }
		const std::vector<uint32_t>& GetTextures() { return m_textures; }
		const std::vector<uint32_t>& GetDrawBuffers() { return m_drawBuffers; }
		bool IsMultisampled() const { return m_multisampled; }
		const bool UseWindowSize() const { return m_useWindowSize; }
		JLEngine::DepthType DepthType() const { return m_depthType; }
		const std::vector<RTParams>& GetTextureAttributes() { return m_attributes; }
		const RTParams& GetTextureAttribute(int index) { return m_attributes[index]; }

		uint32_t GetWidth() const { return m_width; }
		uint32_t GetHeight() const { return m_height; }

		void SetNumSamples(int samples) { m_numSamples = samples; }
		void SetMultisampled(bool multisampled) { m_multisampled = multisampled; }
		void SetUseWindowSize(bool flag) { m_useWindowSize = flag; }
		void SetWidth(uint32_t width) { m_width = width; }
		void SetHeight(uint32_t height) { m_height = height; }
		void SetDepthType(JLEngine::DepthType depthType) { m_depthType = depthType; }
		void SetNumSources(uint32_t numSources);
		void SetTextureAttribute(uint32_t index, const RTParams& attributes);

		void BindDepthTexture(int texUnit) const;
		void BindTexture(int texIndex, int texUnit);

		void ResizeTextures(int newWidth, int newHeight);

	private:

		bool m_useWindowSize;
		JLEngine::DepthType m_depthType;

		uint32_t m_width;
		uint32_t m_height;

		std::vector<RTParams> m_attributes;

		uint32_t m_dbo;	// depth FBO

		bool m_multisampled = false;
		int m_numSamples = 4;

		std::vector<uint32_t> m_textures;	// texture Id's
		std::vector<uint32_t> m_drawBuffers;
		uint32_t m_numSources;
	};
}

#endif