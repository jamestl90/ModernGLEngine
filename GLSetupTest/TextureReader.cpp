#include "TextureReader.h"

namespace JLEngine
{
	TextureReader::TextureReader()
		: m_width(0), m_height(0), m_format(0), m_internalFormat(0), m_success(false)
	{
	}

	TextureReader::~TextureReader()
	{
	}

	bool TextureReader::ReadTexture( const std::string& texture )
	{
		//ilGenImages(1, &m_texture);
		//ilBindImage(m_texture);
		//
		//ILboolean success = ilLoadImage(texture.c_str());
		//
		//if (success)
		//{
		//	m_success = true;
		//}
		//else
		//{
		//	m_success = false;
		//}
		//
		//if (!m_success)
		//{
		//	return false;
		//}
		//
		//m_internalFormat = ilGetInteger(IL_IMAGE_BPP);
		//m_format = ilGetInteger(IL_IMAGE_FORMAT);
		//m_width = ilGetInteger(IL_IMAGE_WIDTH);
		//m_height = ilGetInteger(IL_IMAGE_HEIGHT);

		return m_success;
	}

	void TextureReader::Clear()
	{
		//ilDeleteImage(m_texture);	// free the IL texture data

		m_width = m_height = m_format = m_internalFormat = -1;
	}

	byte* TextureReader::GetData()
	{
		if (m_success)
		{
			return nullptr;
		}
		else
		{
			return nullptr;
		}
		return nullptr;
	}
}