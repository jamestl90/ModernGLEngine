#include "RenderTarget.h"

#include <glad/glad.h>

namespace JLEngine
{
	RenderTarget::RenderTarget(uint32 handle, string& name, string& path)
		: GraphicsResource(handle, name, path), m_fbo(-1), m_dbo(-1), m_sources(nullptr), 
		m_height(0), m_width(0), m_useDepth(false), m_numSources(0), m_useWindowSize(false), m_drawBuffers(nullptr)
	{

	}

	RenderTarget::RenderTarget(uint32 width, uint32 height, bool useDepth, int numSources)
		: GraphicsResource(uint32(0)), m_fbo(-1), m_dbo(-1), m_sources(nullptr),
		m_height(height), m_width(width), m_useDepth(useDepth), m_numSources(numSources), m_useWindowSize(false), m_drawBuffers(nullptr)
	{
		m_sources = new uint32[numSources];
		m_drawBuffers = new uint32[numSources];
		
		for (uint32 i = 0; i < m_numSources; i++)
		{
			m_drawBuffers[i] = GL_COLOR_ATTACHMENT0 + i;
		}
	}

	RenderTarget::~RenderTarget()
	{
		UnloadFromGraphics();
	
		if (m_drawBuffers != nullptr) delete[] m_drawBuffers;
		if (m_sources != nullptr) delete[] m_sources;
	}

	void RenderTarget::Init( Graphics* graphics )
	{
		m_graphics = graphics;

		if (m_useWindowSize)
		{
			m_width = m_graphics->GetWindow()->getWidth();
			m_height = m_graphics->GetWindow()->getHeight();
		}

		m_graphics->CreateRenderTarget(this);
	}

	void RenderTarget::Bind()
	{
		for (uint32 i = 0; i < m_numSources; i++)
		{
			uint32 activeTex = GL_TEXTURE0 + i;
			m_graphics->SetActiveTexture(activeTex);
			m_graphics->BindTexture(GL_TEXTURE_2D, m_sources[i]);
		}
	}

	void RenderTarget::Unbind()
	{
		for (uint32 i = 0; i < m_numSources; i++)
		{
			uint32 activeTex = GL_TEXTURE0 + i;
			m_graphics->SetActiveTexture(activeTex);
			m_graphics->BindTexture(GL_TEXTURE_2D, 0);
		}
	}

	void RenderTarget::SetNumSources( uint32 numSources )
	{
		m_numSources = numSources;

		if (m_sources != nullptr) delete[] m_sources;
		m_sources = new uint32[m_numSources];

		if (m_drawBuffers != nullptr) delete[] m_drawBuffers;
		m_drawBuffers = new uint32[m_numSources];

		for (uint32 i = 0; i < m_numSources; i++)
		{
			m_drawBuffers[i] = GL_TEXTURE0 + i;
		}
	}

	void RenderTarget::UnloadFromGraphics()
	{
		m_graphics->DisposeRenderTarget(this);
	}
}