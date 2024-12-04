#ifndef RENDER_TARGET_H
#define RENDER_TARGET_H

#include "Types.h"
#include "Texture.h"
#include "Buffer.h"

namespace JLEngine
{
	struct TextureAttribute
	{
		GLuint internalFormat = GL_RGBA8; // Internal format (e.g., GL_RGBA8, GL_RGB32F)
		GLuint format =			GL_RGBA;         // Format (e.g., GL_RGBA, GL_RGB)
		GLuint dataType =		GL_UNSIGNED_BYTE; // Data type (e.g., GL_UNSIGNED_BYTE, GL_FLOAT)
		GLuint minFilter =		GL_LINEAR;    // Minification filter
		GLuint magFilter =		GL_LINEAR;    // Magnification filter
		GLuint wrapS =			GL_CLAMP_TO_EDGE; // Wrap mode for S
		GLuint wrapT =			GL_CLAMP_TO_EDGE; // Wrap mode for T
	};

	enum class DepthType 
	{
		None,
		Renderbuffer,
		Texture
	};

	class RenderTarget : public Resource
	{
	public:
		RenderTarget(uint32 handle, const string& name, uint32 numSources);

		~RenderTarget();

		void SetFrameBufferId(uint32 id) { m_fbo = id; }
		void SetDepthId(uint32 id) { m_dbo = id; }
		void SetSourceId(uint32 index, uint32 id) { m_sources[index] = id; }

		const uint32 GetFrameBufferId() const { return m_fbo; }
		const uint32 GetDepthBufferId() const { return m_dbo; }
		int GetSourceId(int index) { return m_sources[index]; }
		const uint32 GetNumSources() const { return m_numSources; }
		const SmallArray<uint32>& GetSources() { return m_sources; }
		const SmallArray<uint32>& GetDrawBuffers() { return m_drawBuffers; }
		const bool UseWindowSize() const { return m_useWindowSize; }
		JLEngine::DepthType DepthType() const { return m_depthType; }
		const SmallArray<TextureAttribute>& GetTextureAttributes() { return m_attributes; }

		uint32 GetWidth() const { return m_width; }
		uint32 GetHeight() const { return m_height; }

		void SetUseWindowSize(bool flag) { m_useWindowSize = flag; }
		void SetWidth(uint32 width) { m_width = width; }
		void SetHeight(uint32 height) { m_height = height; }
		void SetDepthType(JLEngine::DepthType depthType) { m_depthType = depthType; }
		void SetNumSources(uint32 numSources);
		void SetTextureAttribute(uint32 index, const TextureAttribute& attributes);

		void UploadToGPU(Graphics* graphics);
		void UnloadFromGraphics();

		void BindDepthTexture(int texUnit);
		void BindTexture(int texIndex, int texUnit);
		void BindTextures();
		void Unbind() const;

	private:

		bool m_useWindowSize;
		JLEngine::DepthType m_depthType;

		uint32 m_width;
		uint32 m_height;

		SmallArray<TextureAttribute> m_attributes;

		uint32 m_fbo;	// main FBO
		uint32 m_dbo;	// depth FBO

		SmallArray<uint32> m_sources;	// texture Id's
		SmallArray<uint32> m_drawBuffers;
		uint32 m_numSources;

		Graphics* m_graphics;
	};
}

#endif