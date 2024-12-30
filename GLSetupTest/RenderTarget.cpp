#include "RenderTarget.h"
#include "Graphics.h"
#include "GraphicsAPI.h"

#include <glad/glad.h>

namespace JLEngine
{
	RenderTarget::RenderTarget(const string& name)
		: GPUResource(name), m_fbo(0), m_dbo(0), m_numSources(1),
		m_height(0), m_width(0), m_depthType(DepthType::Renderbuffer), m_useWindowSize(false)
	{

	}

	RenderTarget::~RenderTarget()
	{
		if (JLEngine::Graphics::Alive())
			Graphics::DisposeRenderTarget(this);
	}

	void RenderTarget::BindDepthTexture(int texUnit) const
	{
		Graphics::API()->SetActiveTexture(texUnit);
		Graphics::API()->BindTexture(GL_TEXTURE_2D, m_dbo);
	}

	void RenderTarget::BindTexture(int texIndex, int texUnit)
	{
		if (texIndex < 0 || texIndex >= (int)GetNumSources())
		{
			std::cerr << "RenderTarget: Invalid texture index!" << std::endl;
			return;
		}
		GLuint textureId = m_sources[texIndex];
		if (textureId != 0) 
		{
			Graphics::API()->SetActiveTexture(texUnit);
			Graphics::API()->BindTexture(GL_TEXTURE_2D, textureId);
		}
	}

	void RenderTarget::BindTextures()
	{
		for (uint32_t i = 0; i < m_numSources; i++)
		{
			Graphics::API()->SetActiveTexture(i);
			Graphics::API()->BindTexture(GL_TEXTURE_2D, m_sources[i]);
		}
	}

	void RenderTarget::Unbind() const
	{
		for (uint32_t i = 0; i < m_numSources; i++)
		{
			Graphics::API()->SetActiveTexture(i);
			Graphics::API()->BindTexture(GL_TEXTURE_2D, 0);
		}
	}

	void RenderTarget::ResizeTextures(int newWidth, int newHeight)
	{
		if (newWidth == m_width && newHeight == m_height)
			return;

		m_width = newWidth;
		m_height = newHeight;

		Graphics::ResizeRenderTarget(this, newWidth, newHeight);
	}

	void RenderTarget::SetNumSources( uint32_t numSources )
	{
		m_numSources = numSources;

		m_sources.resize(m_numSources);
		m_drawBuffers.resize(m_numSources);
		m_attributes.resize(m_numSources);

		for (uint32_t i = 0; i < m_numSources; i++)
		{
			m_drawBuffers[i] = GL_COLOR_ATTACHMENT0 + i;
		}
	}

	void RenderTarget::SetTextureAttribute(uint32_t index, const TextureAttribute& attributes)
	{
		if (index < m_attributes.size())
		{
			m_attributes[index] = attributes;
		}
		else
		{
			std::cerr << "RenderTarget::SetTextureAttributes: Index out of range!" << std::endl;
		}
	}
}