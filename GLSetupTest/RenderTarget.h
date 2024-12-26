#ifndef RENDER_TARGET_H
#define RENDER_TARGET_H

#include "Types.h"
#include "Texture.h"
#include "Buffer.h"
#include "GraphicsResource.h"

#include <vector>

namespace JLEngine
{
	struct TextureAttribute
	{
		GLuint internalFormat = GL_RGBA8;			// Internal format (e.g., GL_RGBA8, GL_RGB32F)
		GLuint format =			GL_RGBA;			// Format (e.g., GL_RGBA, GL_RGB)
		GLuint dataType =		GL_UNSIGNED_BYTE;	// Data type (e.g., GL_UNSIGNED_BYTE, GL_FLOAT)
		GLuint minFilter =		GL_LINEAR;			// Minification filter
		GLuint magFilter =		GL_LINEAR;			// Magnification filter
		GLuint wrapS =			GL_CLAMP_TO_EDGE;	// Wrap mode for S
		GLuint wrapT =			GL_CLAMP_TO_EDGE;	// Wrap mode for T
	};

	enum class DepthType 
	{
		None,
		Renderbuffer,
		Texture
	};

	class RenderTarget : public GraphicsResource
	{
	public:
		RenderTarget(const string& name, GraphicsAPI* graphics);

		~RenderTarget();

		void SetFrameBufferId(uint32_t id) { m_fbo = id; }
		void SetDepthId(uint32_t id) { m_dbo = id; }
		void SetSourceId(uint32_t index, uint32_t id) { m_sources[index] = id; }

		const uint32_t GetFrameBufferId() const { return m_fbo; }
		const uint32_t GetDepthBufferId() const { return m_dbo; }
		int GetSourceId(int index) { return m_sources[index]; }
		const uint32_t GetNumSources() const { return m_numSources; }
		const std::vector<uint32_t>& GetSources() { return m_sources; }
		const std::vector<uint32_t>& GetDrawBuffers() { return m_drawBuffers; }
		const bool UseWindowSize() const { return m_useWindowSize; }
		JLEngine::DepthType DepthType() const { return m_depthType; }
		const std::vector<TextureAttribute>& GetTextureAttributes() { return m_attributes; }

		uint32_t GetWidth() const { return m_width; }
		uint32_t GetHeight() const { return m_height; }

		void SetUseWindowSize(bool flag) { m_useWindowSize = flag; }
		void SetWidth(uint32_t width) { m_width = width; }
		void SetHeight(uint32_t height) { m_height = height; }
		void SetDepthType(JLEngine::DepthType depthType) { m_depthType = depthType; }
		void SetNumSources(uint32_t numSources);
		void SetTextureAttribute(uint32_t index, const TextureAttribute& attributes);

		void BindDepthTexture(int texUnit);
		void BindTexture(int texIndex, int texUnit);
		void BindTextures();
		void Unbind() const;

		void ResizeTextures(int newWidth, int newHeight);

	private:

		bool m_useWindowSize;
		JLEngine::DepthType m_depthType;

		uint32_t m_width;
		uint32_t m_height;

		std::vector<TextureAttribute> m_attributes;

		uint32_t m_fbo;	// main FBO
		uint32_t m_dbo;	// depth FBO

		std::vector<uint32_t> m_sources;	// texture Id's
		std::vector<uint32_t> m_drawBuffers;
		uint32_t m_numSources;

		GraphicsAPI* m_graphics;
	};
}

#endif