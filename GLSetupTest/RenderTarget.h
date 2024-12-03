#ifndef RENDER_TARGET_H
#define RENDER_TARGET_H

#include "Types.h"
#include "Texture.h"
#include "Buffer.h"

namespace JLEngine
{
	struct TextureAttribute
	{
		uint32 internalFormat = GL_RGBA8; // Internal format (e.g., GL_RGBA8, GL_RGB32F)
		uint32 format =			GL_RGBA;         // Format (e.g., GL_RGBA, GL_RGB)
		uint32 dataType =		GL_UNSIGNED_BYTE; // Data type (e.g., GL_UNSIGNED_BYTE, GL_FLOAT)
		uint32 minFilter =		GL_LINEAR;    // Minification filter
		uint32 magFilter =		GL_LINEAR;    // Magnification filter
		uint32 wrapS =			GL_CLAMP_TO_EDGE; // Wrap mode for S
		uint32 wrapT =			GL_CLAMP_TO_EDGE; // Wrap mode for T
	};

	class RenderTarget : public Resource
	{
	public:
		RenderTarget(uint32 handle, const string& name, uint32 numSources);

		~RenderTarget();

		void SetFrameBufferId(uint32 id) { m_fbo = id; }
		void SetDepthBufferId(uint32 id) { m_dbo = id; }
		void SetSourceId(uint32 index, uint32 id) { m_sources[index] = id; }

		const uint32 GetFrameBufferId() const { return m_fbo; }
		const uint32 GetDepthBufferId() const { return m_dbo; }
		int GetSourceId(int index) { return m_sources[index]; }
		const uint32 GetNumSources() const { return m_numSources; }
		const SmallArray<uint32>& GetSources() { return m_sources; }
		const SmallArray<uint32>& GetDrawBuffers() { return m_drawBuffers; }
		const bool UseWindowSize() const { return m_useWindowSize; }
		bool UseDepth() const { return m_useDepth; }
		const SmallArray<TextureAttribute>& GetTextureAttributes() { return m_attributes; }

		uint32 GetWidth() const { return m_width; }
		uint32 GetHeight() const { return m_height; }

		void SetUseWindowSize(bool flag) { m_useWindowSize = flag; }
		void SetWidth(uint32 width) { m_width = width; }
		void SetHeight(uint32 height) { m_height = height; }
		void SetUseDepth(bool useDepth) { m_useDepth = useDepth; }
		void SetNumSources(uint32 numSources);
		void SetTextureAttribute(uint32 index, const TextureAttribute& attributes);

		void UploadToGPU(Graphics* graphics);
		void UnloadFromGraphics();

		void BindTexture(int texIndex, int texUnit);
		void BindTextures();
		void Unbind();

	private:

		bool m_useWindowSize;
		bool m_useDepth;

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