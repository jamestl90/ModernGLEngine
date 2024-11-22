#include "Texture.h"

namespace JLEngine
{
	Texture::Texture(uint32 handle, string& name, string& path) 
		: GraphicsResource(handle, name, path), m_width(-1), m_height(-1), m_dataType(GL_UNSIGNED_BYTE)
	{

	}

	Texture::Texture(uint32 handle, string& fileName) 
		: GraphicsResource(handle, fileName), m_width(-1), m_height(-1), m_dataType(GL_UNSIGNED_BYTE)
	{
		UnloadFromGraphics();
	}

	Texture::~Texture()
	{
		m_graphics->DisposeTexture(this);
	}

	void Texture::Init(Graphics* graphics)
	{
		m_graphics = graphics;

		m_graphics->CreateTexture(this, m_fromFile);	
	}

	void Texture::UnloadFromGraphics()
	{
		m_graphics->DisposeTexture(this);
	}
}