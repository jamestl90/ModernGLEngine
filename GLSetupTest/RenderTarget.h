#ifndef RENDER_TARGET_H
#define RENDER_TARGET_H

#include "Types.h"
#include "Texture.h"

namespace JLEngine
{
	class RenderTarget : public Resource
	{
	public:
		RenderTarget(uint32 handle, string& name, string& path, int numSources);

		~RenderTarget();

		void SetFrameBufferId(uint32 id) { m_fbo = id; }
		void SetDepthBufferId(uint32 id) { m_dbo = id; }
		void SetSourceId(uint32 index, uint32 id) { m_sources[index] = id; }

		const uint32 GetFrameBufferId() { return m_fbo; }
		const uint32 GetDepthBufferId() { return m_dbo; }
		const uint32 GetSourceId(int index) { return m_sources[index]; }
		const uint32 GetNumSources() { return m_numSources; }
		const uint32* GetSources() { return m_sources; }
		const uint32* GetDrawBuffers() { return m_drawBuffers; }
		const bool UseWindowSize() { return m_useWindowSize; }

		bool UseDepth() { return m_useDepth; }

		uint32 GetWidth() { return m_width; }
		uint32 GetHeight() { return m_height; }

		void SetUseWindowSize(bool flag) { m_useWindowSize = flag; }
		void SetWidth(uint32 width) { m_width = width; }
		void SetHeight(uint32 height) { m_height = height; }
		void SetUseDepth(bool useDepth) { m_useDepth = useDepth; }
		void SetNumSources(uint32 numSources);

		void Init(Graphics* graphics);
		void UnloadFromGraphics();

		void Bind();	
		void Unbind();

	private:

		bool m_useWindowSize;

		bool m_useDepth;

		uint32 m_width;

		uint32 m_height;

		uint32 m_fbo;	// main FBO

		uint32* m_sources;	// texture Id's

		uint32 m_dbo;	// depth FBO

		uint32 m_numSources;

		uint32* m_drawBuffers;

		Graphics* m_graphics;
	};
}

#endif