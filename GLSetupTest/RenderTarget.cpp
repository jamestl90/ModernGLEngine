#include "RenderTarget.h"

#include <glad/glad.h>

namespace JLEngine
{
	RenderTarget::RenderTarget(uint32 handle, const string& name, uint32 numSources)
		: Resource(handle, name), m_fbo(0), m_dbo(0), m_numSources(numSources),
		m_height(0), m_width(0), m_useDepth(false), m_useWindowSize(false), m_graphics(nullptr)
	{
		m_attributes.Create(numSources);
		m_sources.Create(numSources);
		m_drawBuffers.Create(numSources);

		for (uint32 i = 0; i < m_numSources; i++)
		{
			m_drawBuffers[i] = GL_COLOR_ATTACHMENT0 + i;
		}
	}

	RenderTarget::~RenderTarget()
	{
		UnloadFromGraphics();
	}

	void RenderTarget::UploadToGPU( Graphics* graphics )
	{
		m_graphics = graphics;

		if (m_useWindowSize)
		{
			m_width = m_graphics->GetWindow()->GetWidth();
			m_height = m_graphics->GetWindow()->GetHeight();
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

		m_sources.Create(m_numSources);
		m_drawBuffers.Create(m_numSources);

		for (uint32 i = 0; i < m_numSources; i++)
		{
			m_drawBuffers[i] = GL_TEXTURE0 + i;
		}
	}

	void RenderTarget::SetTextureAttribute(uint32 index, const TextureAttribute& attributes)
	{
		if (index < m_attributes.Size())
		{
			m_attributes[index] = attributes;
		}
		else
		{
			std::cerr << "RenderTarget::SetTextureAttributes: Index out of range!" << std::endl;
		}
	}

	void RenderTarget::UnloadFromGraphics()
	{
		m_graphics->DisposeRenderTarget(this);
	}
}